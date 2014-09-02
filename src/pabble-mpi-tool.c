#include <assert.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sesstype/st_node.h>
#include <sesstype/st_node_print.h>
#include <sesstype/st_normalise.h>

#include <scribble/parser.h>
#include <scribble/print.h>
#include <scribble/print_utils.h>

#include "scribble/mpi_print.h"
#include "scribble/pabble_mpi_utils.h"

#define PABBLE_MPI_TOOL_VERSION "1.2.0"

extern int yyparse(st_tree *tree);
extern FILE *yyin;
extern int scribble_codegen_mode;

const char *pabble_mpi_version()
{
  return PABBLE_MPI_TOOL_VERSION;
}

int main(int argc, char *argv[])
{
  int option;
  int show_usage = 0;
  int show_version = 0;
  int verbosity_level = 0;
  char *scribble_file = NULL;
  FILE *output_handle = stdout;

  while (1) {
    static struct option long_options[] = {
      {"output",  required_argument, 0, 'o'},
      {"version", no_argument,       0, 'v'},
      {"verbose", no_argument,       0, 'V'},
      {"help",    no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };

    int option_idx = 0;
    option = getopt_long(argc, argv, "o:vVh", long_options, &option_idx);

    if (option == -1) break;

    switch (option) {
      case 'o':
        if (strcmp(optarg, "--") != 0) {
          output_handle = fopen(optarg, "a");
        }
        break;
      case 'v':
        show_version = 1;
        break;
      case 'V':
        verbosity_level++;
        break;
      case 'h':
        show_usage |= 1;
        break;
    }
  }

  argc -= optind-1;
  argv[optind-1] = argv[0];
  argv += optind-1;

  if (show_version) {
    fprintf(stderr, "%s %s\n", argv[0], PABBLE_MPI_TOOL_VERSION);
    return EXIT_SUCCESS;
  }

  if (argc < 2) {
    show_usage |= 1;
  }

  if (show_usage) {
    fprintf(stderr, "Usage: %s [--output] [-v] [-h] Scribble.spr\n", argv[0]);
    return EXIT_SUCCESS;
  }

  scribble_file = argv[1];
  if (strcmp(argv[1], "-") == 0) {
    yyin = stdin;
  } else {
    yyin = fopen(scribble_file, "r");
  }
  if (yyin == NULL) {
    perror(scribble_file);
    return EXIT_FAILURE;
  }

  st_tree *tree = st_tree_init((st_tree *)malloc(sizeof(st_tree)));
  if (0 != yyparse(tree)) {
    fprintf_error(stderr, "Parse failed\n");
    return EXIT_FAILURE;
  }

  if (tree->root != 0) {
    st_node_normalise(tree->root);
    if (verbosity_level > 2) st_tree_print(tree);
    if (tree->info->type == ST_TREE_GLOBAL) {
      if (output_handle != NULL) fclose(output_handle);
      output_handle = NULL;
      fprintf_error(stderr, "Cannot generate code for global protocol\n");
    }
    if (output_handle != NULL) {
      if (verbosity_level > 0) fprintf_info(stderr, "Writing MPI\n");
      mpi_print(output_handle, tree);
      if (output_handle != stdout) {
        fclose(output_handle);
      }
    }
  }

  st_tree_free(tree);

  return EXIT_SUCCESS;
}
