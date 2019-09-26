/*
*    EOS - Experimental Operating System
*    ELF tools module source file
*/
#include <kernel/pm/elf.h>
#include <kernel/fs/vfs.h>
#include <kernel/mm/kheap.h>
#include <kernel/tty.h>

#include <libk/string.h>


//Returns 0 if header is valid, 1 if magic number invalid, 2 and more if file isn't compatible
uint8_t elf_check_header(struct elf_hdr *hdr)
{
	if (hdr->mag_num[0] != 0x7f || hdr->mag_num[1] != 'E' || hdr->mag_num[2] != 'L' || hdr->mag_num[3] != 'F')
		return 1; //our magic number isn't valid.
	if (hdr->arch != ELF_ARCH_32BIT)
		return 2; //This ELF file can't be loaded because it's not 32-bit.
	if (hdr->byte_order != ELF_BYTEORDER_LENDIAN)
		return 3; //This ELF file can't be loaded because it's not little-endian.
	if (hdr->elf_ver != 1)
		return 4; //This ELF file can't be loaded because its version isn't 1.
	if (hdr->file_type != ELF_REL && hdr->file_type != ELF_EXEC)
		return 5; //This ELF file can't be loaded because it's neither executable nor relocatable file.
	if (hdr->machine!=ELF_386_MACHINE)
		return 6; //This ELF file can't be loaded because it's not for i386 platform.
	return 0;
}


void *elf_open(const char* fname) //Returns pointer to ELF file.
{
	if (!vfs_exists(fname))
	{
		tty_printf("elf doesnt exist\n");
		return 0;
	}
	uint32_t fsize = vfs_get_size(fname);
	//tty_printf("elf size = %d\n", fsize);
	void *addr = kheap_malloc(fsize);
	//tty_printf("addr = %x\n", addr);
	int res = vfs_read(fname, 0, fsize, addr);
	struct elf_hdr *hdr = addr;
	//uint8_t status = elf_check_header(hdr);
	//if(status)
	//{
	//	return 0;
	//}
	/*else */return hdr;
}


struct elf_section_header *elf_get_section_header(void *elf_file, int num)
{
	struct elf_hdr *hdr = (struct elf_hdr*)elf_file;
	return (struct elf_section_header *)(elf_file + hdr->shoff + hdr->sh_ent_size*num);
}


struct elf_program_header *elf_get_program_header(void *elf_file, int num)
{
	struct elf_hdr *hdr = (struct elf_hdr*) elf_file;
	return (struct elf_program_header *)(elf_file + hdr->phoff + hdr->ph_ent_size*num);
}


const char *elf_get_section_name(void *elf_file, int num)
{
	struct elf_hdr *hdr = (struct elf_hdr*)elf_file;
	return (hdr->sh_name_index == SH_UNDEF) ? "no section" : (const char*)elf_file + elf_get_section_header(elf_file, hdr->sh_name_index)->offset + elf_get_section_header(elf_file, num)->name;
}


void elf_hdr_info(struct elf_hdr *hdr)
{
	tty_printf("\tHeader information:\n");
	tty_printf("\t\tArchitecture: %s\n", (hdr->arch==ELF_ARCH_32BIT) ? "32-bit" : (hdr->arch==ELF_ARCH_64BIT) ? "64-bit" : "Unknown architecture");
	tty_printf("\t\tByte order: %s\n", (hdr->byte_order==ELF_BYTEORDER_LENDIAN) ? "little-endian" : "unknown");
	tty_printf("\t\tELF file version: %u\n", hdr->elf_ver);
	tty_printf("\t\tELF file type: %s\n", (hdr->file_type==ELF_REL) ? "relocatable" : (hdr->file_type==ELF_EXEC) ? "executable" : "unknown");
	tty_printf("\t\tTarget machine: %s\n", (hdr->machine==ELF_386_MACHINE) ? "i386" : "unknown");
	tty_printf("\t\tEntry point: 0x%08X\n", hdr->entry);
	tty_printf("\t\tProgram header offset: %u\n", hdr->phoff);
	tty_printf("\t\tSection header offset: %u\n", hdr->shoff);
	tty_printf("\t\tFile flags: %u\n", hdr->flags);
	tty_printf("\t\tFile header size: %u\n", hdr->hsize);
	tty_printf("\t\tProgram header entry size: %u\n", hdr->ph_ent_size);
	tty_printf("\t\tSection header entry size: %u\n", hdr->sh_ent_size);
	tty_printf("\t\tSection header count: %d\n", hdr->sh_ent_cnt);
	tty_printf("\t\tProgram header count: %d\n", hdr->ph_ent_cnt);
}


void elf_info_short(const char* name)
{
	void *elf_file = elf_open(name);
	if (elf_file == NULL)
		return;
	struct elf_hdr *hdr=(struct elf_hdr *)elf_file;
	tty_printf("\tName: %s\n", name);
	tty_printf("\tFile size: %u\n\tELF file info:\n", vfs_get_size(name));
	elf_hdr_info(hdr);
	tty_printf("\t\tSection list:\n");
	int i;
	for(i = 1; i<hdr->sh_ent_cnt; i++)
	{
		struct elf_section_header *shdr=(struct elf_section_header *)(elf_file+hdr->shoff+hdr->sh_ent_size*i);
		tty_printf("\t\t\tSection %d: name: %s, data offset: %u.\n", i, elf_get_section_name(elf_file, i), shdr->offset, shdr->name, i);
	}
}


void elf_info(const char* name)
{
	void *elf_file = elf_open(name);
	tty_printf("pointer to this elf_file = %x\n", elf_file);
	if(elf_file == NULL)
		return;
	struct elf_hdr *hdr = (struct elf_hdr *)elf_file;
	tty_printf("\tName: %s\n", name);
	tty_printf("\tFile size: %u\n\tELF file info:\n", vfs_get_size(name));
	elf_hdr_info(hdr);
	int i;
	for (i = 0; i < hdr->sh_ent_cnt; i++)
	{
		struct elf_section_header *shdr = (struct elf_section_header *)(elf_file + hdr->shoff + hdr->sh_ent_size*i);
		tty_printf("\t\t\tSection %d:\n", i);
		tty_printf("\t\t\t\tActual section offset: %u\n\t\t\t\tSection name offset in string table: %d\n", shdr->offset, shdr->name);
		tty_printf("\t\t\t\tSection name: %s\n", elf_get_section_name(elf_file, i));
	}
	//kheap_free(elf_file);  //!!!!!!
}