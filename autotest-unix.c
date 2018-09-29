#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AUTOTEST_BUILDING
#include "./autotest.h"

#if __BYTE_ORDER == __LITTLE_ENDIAN
#	define ELFDATANATIVE ELFDATA2LSB
#elif __BYTE_ORDER == __BIG_ENDIAN
#	define ELFDATANATIVE ELFDATA2MSB
#else
#	error "Unknown machine endian"
#endif

#if __GNUC__
#	if __x86_64__ || __ppc64__
		typedef Elf64_Sym sym_ent_t;
		typedef Elf64_Ehdr elf_header_t;
		typedef Elf64_Shdr section_header_t;
		typedef Elf64_Dyn dyn_ent_t;
#		define ELF_ST_TYPE ELF64_ST_TYPE
		typedef uint64_t bloom_el_t;
#	else
		typedef Elf32_Sym sym_ent_t;
		typedef Elf32_Ehdr elf_header_t;
		typedef Elf32_Shdr section_header_t;
		typedef Elf32_Dyn dyn_ent_t;
#		define ELF_ST_TYPE ELF32_ST_TYPE
		typedef uint32_t bloom_el_t;
#	endif
#endif

static void discover_symbols(const char *arg0, test_case **cases, const dyn_ent_t *dynent, void *base_addr) {
	const sym_ent_t *symtab = NULL;
	const char *strtab = NULL;
	void *hashtable = NULL;
	int hashtable_type = 0;
	size_t i;
	size_t num_entries = 0;
	test_case *tcase = NULL;

	(void) arg0; /* not needed - we pull symbols directly from memory */

	*cases = NULL;


	/*
		TODO fixme The pointer offset branch (where we add base_addr below)
		is an unreliable hack.

		I can't figure out the logic here to determine if
		you need to add the base address or not. On some systems
		in ubuntu, you don't. Most other systems, you do.
		In the man pages, you do. I think it's a bug in an implementation
		somewhere due to poor standardization of the hashtables / ELF format.

		If you know more about this, please let me know. Check the Dockerfile
		and see that removing the offset addition breaks Alpine and adding it
		(unconditionally) breaks Ubuntu. Further, base_addr is 0x0 on my vbox
		Ubuntu machine I was originally testing on.

		Note to system developers: this is why people are scared of native.
	*/

	for (; dynent->d_tag != DT_NULL; ++dynent) {
		switch (dynent->d_tag) {
		case DT_HASH:
			hashtable_type = 1;
			hashtable = (size_t) dynent->d_un.d_ptr < (size_t) base_addr
				? &(((char *)base_addr)[dynent->d_un.d_ptr])
				: (void*) dynent->d_un.d_ptr;
			break;
		case DT_GNU_HASH:
			hashtable_type = 2;
			hashtable = (size_t) dynent->d_un.d_ptr < (size_t) base_addr
				? &(((char *)base_addr)[dynent->d_un.d_ptr])
				: (void*) dynent->d_un.d_ptr;
			break;
		case DT_STRTAB:
			strtab = (size_t) dynent->d_un.d_ptr < (size_t) base_addr
				? &(((char *)base_addr)[dynent->d_un.d_ptr])
				: (void*) dynent->d_un.d_ptr;
			break;
		case DT_SYMTAB:
			symtab = (size_t) dynent->d_un.d_ptr < (size_t) base_addr
				? &(((char *)base_addr)[dynent->d_un.d_ptr])
				: (void*) dynent->d_un.d_ptr;
			break;
		default:
			break;
		}
	}

	if (symtab == NULL) bailout("Main executable has no symbol table (spooky things are happening)");
	if (strtab == NULL) bailout("Main executable has no string table (spooky things are happening)");
	if (hashtable_type == 0) bailout("Main executable has no symbol hash table (spooky things are happening)");

	if (hashtable_type == 1) {
		num_entries = ((const uint32_t *) hashtable)[1];
	} else {
		/* https://flapenguin.me/2017/05/10/elf-lookup-dt-gnu-hash/ */
		/* https://chromium-review.googlesource.com/c/crashpad/crashpad/+/876879/18/snapshot/elf/elf_image_reader.cc#714 */
		const uint32_t nbuckets = ((uint32_t *)hashtable)[0];
		const uint32_t symoffset = ((uint32_t *)hashtable)[1];
		const uint32_t bloom_size = ((uint32_t *)hashtable)[2];
		const bloom_el_t* bloom = (void*)&(((uint32_t*)hashtable)[4]);
		const uint32_t* buckets = (void*)&bloom[bloom_size];
		const uint32_t* chain = &buckets[nbuckets];
		uint32_t chain_entry;

		uint32_t max_bucket = 0;
		for (i = 0; i < nbuckets; i++) {
			if (buckets[i] > max_bucket) {
				max_bucket = buckets[i];
			}
		}

		if (max_bucket < symoffset) {
			num_entries = symoffset;
		} else {
			while (1) {
				chain_entry = chain[max_bucket - symoffset];
				++max_bucket;
				if ((chain_entry & 1)) {
					break;
				}
			}

			num_entries = max_bucket;
		}
	}

	for (i = 0; i < num_entries; i++) {
		const char *symname = &strtab[symtab[i].st_name];

		if (strncmp(&symname[*symname == '_'], "TEST_", 5) == 0) {
			test_case_fn fn;
			test_case *c = malloc(sizeof(*c));

			assert(c != NULL);

#			pragma GCC diagnostic push
#			pragma GCC diagnostic ignored "-Wpedantic"
#			pragma GCC diagnostic ignored "-Wpointer-arith"
			fn = (void *)(base_addr + symtab[i].st_value);
#			pragma GCC diagnostic pop

			if (populate_test_case(c, symname, fn) != 0) {
				/* symbol is malformed */
				free(c);
				continue;
			}

			if (*cases == NULL) {
				(*cases) = c;
			} else {
				tcase->next = c;
			}

			tcase = c;
		}
	}
}

void discover_tests(const char *arg0, test_case **cases) {
	struct link_map *map = dlopen(NULL, RTLD_LAZY);
	if (map == NULL) bailout("Could not open main EXE dl object: %s", dlerror());
	while (map->l_prev) map = map->l_prev;
	discover_symbols(arg0, cases, map->l_ld, (void *) map->l_addr);
}
