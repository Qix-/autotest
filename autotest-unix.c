#define _GNU_SOURCE
#include <assert.h>
#include <elf.h>
#include <link.h>
#include <stdint.h>
#include <stdio.h>

typedef void (test_case)(void);

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

static int discover_symbols(const char *arg0, test_case **cases, const dyn_ent_t *dynent) {
	int status = 1;
	const sym_ent_t *symtab = NULL;
	const char *strtab = NULL;

	void *hashtable;
	int hashtable_type = 0;
	size_t i;
	size_t num_entries = 0;

	(void) cases; /* XXX DEBUG */

	for (; dynent->d_tag != DT_NULL; ++dynent) {
		switch (dynent->d_tag) {
		case DT_HASH:
			hashtable_type = 1;
			hashtable = (void *) dynent->d_un.d_ptr;
			break;
		case DT_GNU_HASH:
			hashtable_type = 2;
			hashtable = (void *) dynent->d_un.d_ptr;
			break;
		case DT_STRTAB:
			strtab = (void *) dynent->d_un.d_ptr;
			break;
		case DT_SYMTAB:
			symtab = (void *) dynent->d_un.d_ptr;
			break;
		case DT_RELA:
			puts("RELA");
			break;
		case DT_REL:
			puts("REL");
			break;
		case DT_RELACOUNT:
			puts("DT_RELACOUNT");
			break;
		default:
			break;
		}
	}

	if (symtab == NULL) {
		fprintf(stderr, "%s: error: main executable has no symbol table (spooky things are happening)\n", arg0);
		goto exit;
	}

	if (strtab == NULL) {
		fprintf(stderr, "%s: error: main executable has no string table (spooky things are happening)\n", arg0);
		goto exit;
	}

	if (hashtable_type == 0) {
		fprintf(stderr, "%s: error: main executable has no symbol hash table (spooky things are happening)\n", arg0);
		goto exit;
	}

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
		printf("\t%u\t%u\t%s\n", ELF32_ST_TYPE(symtab[i].st_info), symtab[i].st_name, &strtab[symtab[i].st_name]);
	}

	status = 0;
exit:
	return status;
}

int discover_tests(const char *arg0, test_case **cases) {
	int status = 1;
	struct link_map *map;

	map = dlopen(NULL, RTLD_LAZY);
	if (map == NULL) {
		fprintf(stderr, "%s: error: could not open main EXE dl object: %s\n", arg0, dlerror());
		goto exit;
	}

	while (map->l_prev) map = map->l_prev;

	if (discover_symbols(arg0, cases, map->l_ld) != 0) {
		/* error already emitted */
		goto exit;
	}

	status = 0;
exit:
	return status;
}
