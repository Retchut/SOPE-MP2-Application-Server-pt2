// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include "../src/cmd_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void printUsage(const char *programName) {
  fprintf(stderr, "Usage: %s <-t nsecs> <fifoname>\n", programName);
}

int cmdParser(int argc, char * const argv[], int *nsecs, char **fifoname) {
  if (argc != 3 && argc != 4) {
    fprintf(stderr, "Incorrect number of arguments\n");
    printUsage(argv[0]);
    return 1;
  }

  *fifoname = argv[argc - 1];

  int opt;
  while ((opt = getopt(argc - 1, argv, "t:")) != -1) {
    switch (opt) {
    case 't':
      *nsecs = atoi(optarg);
      break;
    default:
      printUsage(argv[0]);
      return 1;
    }
  }

  return 0;
}
