// COPYRIGHT 2021 Flávio Lobo Vaz, José Costa, Mário Travassos, Tomás Fidalgo


#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_

#define EXTRA_SECS 2

/**
 * @brief Records the time the program started in in a global variable
 * @param duration - Duration of the program run
 * @details The variable holds the time in the number of seconds since the epoch
 */
#include <stdint.h>
void setTimer(int duration);

/**
 * @brief Calculates and returns the number of elapsed seconds since the start of the program
 * @return Number of seconds since the start of the program's execution
 */
int64_t getElapsed();

/**
 * @brief Returns the current time
 * @details The time comes in seconds since the epoch
 * @return The current time
 */
int64_t getTime();


/**
 * @brief Returns the time remaining in this program's execution
 * @details The time comes in seconds
 * @return The current time remaining
 */
int64_t getRemaining();


/**
 * @brief Returns the time remaining in this program's execution, including the extra seconds
 * @details The time comes in seconds, and it includes the extra seconds
 * @return The current time remaining
 */
int64_t getServerRemaining();

#endif  // SRC_TIMER_H_
