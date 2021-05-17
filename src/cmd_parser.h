#ifndef SRC_CMD_PARSER_H_
#define SRC_CMD_PARSER_H_
void printUsage(const char* programName);
int cmdParser(int argc, char* const argv[], int* nsecs, char** fifoname);
#endif /* SRC_CMD_PARSER_H_ */