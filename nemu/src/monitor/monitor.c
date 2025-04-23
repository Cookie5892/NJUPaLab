/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>
#include <elf.h>

void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm();

static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NEMU!\n", ANSI_FMT(str(__GUEST_ISA__), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
  //Log("Exercise: Please remove me in the source code and compile NEMU again.");
  //assert(0)
}

#ifndef CONFIG_TARGET_AM
#include <getopt.h>

void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static int difftest_port = 1234;




static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  assert(ret == 1);

  fclose(fp);
  return size;
}

#ifdef CONFIG_FTRACE_N_Y
//elf文件的读取
typedef struct {
  Elf32_Ehdr *ehdr;
  Elf32_Shdr *shdr_table;
  char *shstrtab;       //shstrtab文件结构是字符串拼形成的字符数组，不是字符指针数组
  Elf32_Sym *symtab;
  int symtab_Nmu;
  char *strtab;

}ElfFile;
static ElfFile *elf_file;
/// 获取符号类型的字符串表示
static const char *get_symbol_type(unsigned char type) {
  switch (type) {
    case STT_NOTYPE: return "NOTYPE";
    case STT_OBJECT: return "OBJECT";
    case STT_FUNC:   return "FUNC";
    case STT_SECTION:return "SECTION";
    case STT_FILE:   return "FILE";
    default:         return "UNKNOWN";
  }
}
// 获取符号绑定的字符串表示
static const char *get_symbol_bind(unsigned char bind) {
  switch (bind) {
    case STB_LOCAL:  return "LOCAL";
    case STB_GLOBAL: return "GLOBAL";
    case STB_WEAK:   return "WEAK";
    default:         return "UNKNOWN";
  }
}
// 获取符号可见性的字符串表示
static const char *get_symbol_vis(unsigned char vis) {
  switch (vis) {
    case STV_DEFAULT:   return "DEFAULT";
    case STV_INTERNAL:  return "INTERNAL";
    case STV_HIDDEN:    return "HIDDEN";
    case STV_PROTECTED: return "PROTECTED";
    default:            return "UNKNOWN";
  }
}
static const char *get_symbol_ndx(Elf32_Section ndx){
  switch(ndx){
    case SHN_UNDEF:   return "UND";
    case SHN_ABS  :   return "ABS";
    default: {static char str[16]; sprintf(str, "%d", ndx);   return str;}
  }
}
// 打印符号表
void print_symbol_table() {
  printf("   Num:    Value  Size Type    Bind   Vis      Ndx  Name\n");
  for (int i = 0; i < elf_file->symtab_Nmu; i++) {
    Elf32_Sym *sym = &elf_file->symtab[i];
    const char *type = get_symbol_type(ELF32_ST_TYPE(sym->st_info));//高四位
    const char *bind = get_symbol_bind(ELF32_ST_BIND(sym->st_info));//低四位
    const char *vis = get_symbol_vis(sym->st_other);
    const char *ndx = get_symbol_ndx(sym->st_shndx);
    // 根据符号类型选择字符串表
    const char *name;
    if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION) {
      name = &elf_file->shstrtab[sym->st_name];
    } else {
      name = &elf_file->strtab[sym->st_name];
    }
    // 打印符号表的每一行
    printf("%5d: %08x %5u %-7s %-6s %-8s %-4s  %s\n",
          i, sym->st_value, sym->st_size, type, bind, vis, ndx, name);
  }
}
// 打印节头
void print_section_headers() {
  printf("Section Headers:\n");
  printf("  [Nr] Name              Type            Addr     Off    Size   ES Flg Lk Inf Al\n");

  for (int i = 0; i < elf_file->ehdr->e_shnum; i++) {
      Elf32_Shdr *shdr = &elf_file->shdr_table[i];
      const char *name = &elf_file->shstrtab[shdr->sh_name]; // 从 shstrtab 获取节名称

      // 获取节类型的字符串表示
      const char *type;
      switch (shdr->sh_type) {
          case SHT_NULL:     type = "NULL"; break;
          case SHT_PROGBITS: type = "PROGBITS"; break;
          case SHT_SYMTAB:   type = "SYMTAB"; break;
          case SHT_STRTAB:   type = "STRTAB"; break;
          case SHT_RELA:     type = "RELA"; break;
          case SHT_HASH:     type = "HASH"; break;
          case SHT_DYNAMIC:  type = "DYNAMIC"; break;
          case SHT_NOTE:     type = "NOTE"; break;
          case SHT_NOBITS:   type = "NOBITS"; break;
          case SHT_REL:      type = "REL"; break;
          case SHT_SHLIB:    type = "SHLIB"; break;
          case SHT_DYNSYM:   type = "DYNSYM"; break;
          default:           type = "UNKNOWN"; break;
      }

      // 打印节头信息
      printf("  [%2d] %-17s %-15s %08x %06x %06x %02x %-1s %-1s %-1s %-1d %-1d %-1d\n",
            i, name, type, shdr->sh_addr, shdr->sh_offset, shdr->sh_size,
            shdr->sh_entsize,
            (shdr->sh_flags & SHF_ALLOC ? "A" : ""),
            (shdr->sh_flags & SHF_EXECINSTR ? "X" : ""),
            (shdr->sh_flags & SHF_WRITE ? "W" : ""),
            shdr->sh_link, shdr->sh_info, shdr->sh_addralign);
  }
}
//ftrace
char *ftrace(vaddr_t pc){
  for (int i = 0; i < elf_file->symtab_Nmu; i++){
    if (ELF32_ST_TYPE(elf_file->symtab[i].st_info) == STT_FUNC ){
      if (pc >= elf_file->symtab[i].st_value && pc < (elf_file->symtab[i].st_value + elf_file->symtab[i].st_size))
      return &elf_file->strtab[elf_file->symtab[i].st_name];//返回字符串的起始地址，
    }
  }
  return "???";
}
//读取elf文件
static void read_elf( const char *elfname){
  if (elfname == NULL){
    Log("Unable to open elf file.");
    return;
  }
  if (elf_file == NULL) {
    elf_file = malloc(sizeof(ElfFile));
    assert(elf_file != NULL);
  }
  FILE *fp = fopen(elfname, "rb");
  Assert(fp, "Can not open elf '%s'", elfname);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);
  Log("The elf is %s, size = %ld", elfname, size);

  fseek(fp, 40, SEEK_SET);
//从elf头读取elf头的大小
Elf32_Half *e_ehsize = malloc(sizeof(Elf32_Half));
  int ret1 = fread(e_ehsize, sizeof(Elf32_Half), 1, fp);
  assert(ret1 == 1);


  fseek(fp, 0, SEEK_SET);
//为elf头分配内存可可空间
elf_file->ehdr = malloc(sizeof(Elf32_Ehdr));
int ret = fread(elf_file->ehdr, sizeof(Elf32_Ehdr), 1, fp);
assert(ret == 1);
printf("从elf头读取elf头的大小: %hu--->%zu\n", *e_ehsize, sizeof(Elf32_Ehdr));
free(e_ehsize);


fseek(fp, elf_file->ehdr->e_shoff, SEEK_SET);
//为节头分配内存空间
elf_file->shdr_table = malloc(elf_file->ehdr->e_shentsize * elf_file->ehdr->e_shnum);
int ret2 = fread(elf_file->shdr_table, elf_file->ehdr->e_shentsize * elf_file->ehdr->e_shnum, 1, fp);
assert(ret2 == 1);

fseek(fp, elf_file->shdr_table[elf_file->ehdr->e_shstrndx].sh_offset, SEEK_SET);
//为shstrtab分配内存空间,shstrtab是字节数组
elf_file->shstrtab = malloc(elf_file->shdr_table[elf_file->ehdr->e_shstrndx].sh_size);
int ret3 = fread(elf_file->shstrtab, elf_file->shdr_table[elf_file->ehdr->e_shstrndx].sh_size, 1, fp);
assert(ret3 == 1);

//为symtab分配内存可空间
for (int i = 0; i < elf_file->ehdr->e_shnum; i++){
  if ( elf_file->shdr_table[i].sh_type == SHT_SYMTAB ){
      fseek(fp, elf_file->shdr_table[i].sh_offset, SEEK_SET);
      elf_file->symtab = malloc(elf_file->shdr_table[i].sh_size);
      int ret4 = fread(elf_file->symtab, elf_file->shdr_table[i].sh_size, 1, fp);
      assert(ret4 == 1);
      elf_file->symtab_Nmu = elf_file->shdr_table[i].sh_size / sizeof(Elf32_Sym);
      //为strtab分配内存空间
      Elf64_Word sh_link = elf_file->shdr_table[i].sh_link;
      fseek(fp, elf_file->shdr_table[sh_link].sh_offset, SEEK_SET);
      elf_file->strtab = malloc(elf_file->shdr_table[sh_link].sh_size);
      int ret5 = fread(elf_file->strtab, elf_file->shdr_table[sh_link].sh_size, 1, fp);
      assert(ret5 == 1);
      break;
  }
}
fclose(fp);
print_symbol_table();
print_section_headers();
}

#endif



static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {"elf"      , required_argument, NULL, 'e'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:e:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 'e': IFDEF(CONFIG_FTRACE_N_Y, read_elf(optarg)); break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-e,--elf                read elf file\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  /*解释命令行参数*/
  parse_args(argc, argv);

  /* Set random seed. */
  init_rand();

  /* Open the log file. */
  init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

  IFDEF(CONFIG_ITRACE, init_disasm());

  /* Display welcome message. */
  welcome();
}
#else // CONFIG_TARGET_AM
static long load_img() {
  extern char bin_start, bin_end;
  size_t size = &bin_end - &bin_start;
  Log("img size = %ld", size);
  memcpy(guest_to_host(RESET_VECTOR), &bin_start, size);
  return size;
}

void am_init_monitor() {
  init_rand();
  init_mem();
  init_isa();
  load_img();
  IFDEF(CONFIG_DEVICE, init_device());
  welcome();
}
#endif
