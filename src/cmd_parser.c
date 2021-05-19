// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo

#include "../src/cmd_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

void printUsage(const char *programName) {
  fprintf(stderr, "Usage: %s <-t nsecs> [-l bufsz] <fifoname>\n", programName);
}

int cmdParser(int argc, char * const argv[], int *nsecs, int *bufsz, char **fifoname) {
  if (argc < 3 && argc > 4) {
    fprintf(stderr, "Incorrect number of arguments\n");
    printUsage(argv[0]);
    return 1;
  }

  *fifoname = argv[argc - 1];

  int opt;
  bool tOpt = false;
  while ((opt = getopt(argc - 1, argv, "t:l:")) != -1) {
    switch (opt) {
    case 't':
      *nsecs = atoi(optarg);
      tOpt = true;
      break;
    case 'l':
      *bufsz = atoi(optarg);
      break;
    default:
      printUsage(argv[0]);
      return 1;
    }
  }

  if(!tOpt){
    printUsage(argv[0]);
    return 1;
  }

  return 0;
}
