#define _GNU_SOURCE
#include <assert.h>
#include <byteswap.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
#	else
		typedef Elf32_Sym sym_ent_t;
		typedef Elf32_Ehdr elf_header_t;
		typedef Elf32_Shdr section_header_t;
#	endif
#endif

int discover_tests(const char *arg0, test_case **cases) {
	const char *elf = NULL;
	const elf_header_t *elf_header = NULL;
	const sym_ent_t *symtab = NULL;
	size_t symtab_count = 0;
	const section_header_t *section_header = NULL;
	struct stat exestat;
	const char *sec_strtab = NULL;
	const char *strtab = NULL;
	const char *exepath = "/proc/self/exe";
	int elffd = -1;
	int status = 1;
	size_t i;
	const char *symname;
	void *symaddr;

	(void) cases; /* XXX DEBUG */

	elffd = open(exepath, O_RDONLY);
	if (elffd == -1) {
		fprintf(stderr, "%s: error: could not open ELF exe for reading: %s: %s\n", arg0, strerror(errno), exepath);
		goto exit;
	}

	if (fstat(elffd, &exestat) == -1) {
		fprintf(stderr, "%s: error: could not fstat ELF exe: %s: %s\n", arg0, strerror(errno), exepath);
		goto close_elf;
	}

	elf_header = mmap(NULL, exestat.st_size, PROT_READ, MAP_PRIVATE, elffd, 0);
	elf = (const char *) elf_header;
	if (elf_header == MAP_FAILED) {
		fprintf(stderr, "%s: error: could not map ELF exe: %s: %s\n", arg0, strerror(errno), exepath);
		goto close_elf;
	}

	if (elf_header->e_ident[EI_MAG0] != ELFMAG0
	    || elf_header->e_ident[EI_MAG1] != ELFMAG1
	    || elf_header->e_ident[EI_MAG2] != ELFMAG2
	    || elf_header->e_ident[EI_MAG3] != ELFMAG3) {
		fprintf(stderr, "%s: error: main binary has invalid ELF header (spooky things are happening)\n", arg0);
		goto unmap_elf;
	}

	if (elf_header->e_shoff == 0) {
		fprintf(stderr, "%s: error: main binary has no section headers (spooky things are happening)\n", arg0);
		goto unmap_elf;
	}

	section_header = (void *)(elf + elf_header->e_shoff);

	if (elf_header->e_shstrndx == SHN_UNDEF) {
		fprintf(stderr, "%s: error: main binary has no section header names (was the file stripped?)\n", arg0);
		goto unmap_elf;
	}

	sec_strtab = (void *)(elf + section_header[elf_header->e_shstrndx].sh_offset);

	for (i = 0; i < elf_header->e_shnum; i++) {
		switch (section_header[i].sh_type) {
		case SHT_SYMTAB:
			symtab = (void *)(elf + section_header[i].sh_offset);
			assert((section_header[i].sh_size % section_header[i].sh_entsize) == 0);
			symtab_count = section_header[i].sh_size / section_header[i].sh_entsize;
			break;
		case SHT_STRTAB:
			if (strcmp(&sec_strtab[section_header[i].sh_name], ".strtab") == 0) {
				strtab = (void *)(elf + section_header[i].sh_offset);
			}
			break;
		default:
			break;
		}
	}

	if (symtab == NULL) {
		fprintf(stderr, "%s: error: could not find symbol table in ELF header (spooky things are happening)\n", arg0);
		goto unmap_elf;
	}

	if (strtab == NULL) {
		fprintf(stderr, "%s: error: could not find string table in ELF header (spooky things are happening)\n", arg0);
		goto unmap_elf;
	}

	for (i = 0; i < symtab_count; i++) {
		if (symtab[i].st_name == 0 || !(symtab[i].st_info & STT_FUNC)) {
			continue;
		}

		symname = &strtab[symtab[i].st_name];
		if (strncmp(&symname[*symname == '_'], "TEST_", 5) == 0) {
			symaddr = dlsym(RTLD_DEFAULT, symname);
			if (symaddr == NULL) {
				fprintf(stderr, "%s: warning: could not get address of test case: %s: %s\n", arg0, dlerror(), symname);
				continue;
			}
			printf("%s\x1b[30G%p\n", symname, symaddr);
		}
	}

	status = 0;
unmap_elf:
	munmap((void *) elf_header, exestat.st_size);
close_elf:
	close(elffd);
exit:
	return status;
}
