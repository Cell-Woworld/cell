/**
 * @brief  Macros to get integer build timestamp:
 *         __DATE_TIME_Y2K__  seconds since 2000-01-01,00:00:00
 *         __DATE_TIME_UNIX__ seconds since 1970-01-01 00:00:00
 *
 *           01234567890              01234567
 * __DATE__ "Jul 27 2019"   __TIME__ "12:34:56"
 */
#ifndef __DATE_TIME_H__
#define __DATE_TIME_H__

#define Y2K_UNIX_EPOCH_DIFF 946684800U
#define YEARS ((__DATE__[10] - '0' + (__DATE__[9] - '0') * 10))
#define DAY_OF_MONTH ((__DATE__[5] - '0') \
                  + (((__DATE__[4] > '0')? __DATE__[4] - '0': 0) * 10) - 1)
#define DAY_OF_YEAR ((DAY_OF_MONTH) + \
( /* Jan */ (__DATE__[0] == 'J' && __DATE__[1] == 'a')?   0: \
  /* Feb */ (__DATE__[0] == 'F'                      )?  31: \
  /* Mar */ (__DATE__[0] == 'M' && __DATE__[2] == 'r')?  59: \
  /* Apr */ (__DATE__[0] == 'A' && __DATE__[1] == 'p')?  90: \
  /* May */ (__DATE__[0] == 'M'                      )? 120: \
  /* Jun */ (__DATE__[0] == 'J' && __DATE__[2] == 'n')? 151: \
  /* Jul */ (__DATE__[0] == 'J'                      )? 181: \
  /* Aug */ (__DATE__[0] == 'A'                      )? 212: \
  /* Sep */ (__DATE__[0] == 'S'                      )? 243: \
  /* Oct */ (__DATE__[0] == 'O'                      )? 273: \
  /* Nov */ (__DATE__[0] == 'N'                      )? 304: \
  /* Dec */                                             334  ))
#define LEAP_DAYS (YEARS / 4 + 1 + ((YEARS % 4 == 0 && DAY_OF_YEAR > 58)? 1 : 0) )
#define __DATE_TIME_Y2K__ ( (YEARS * 365 + LEAP_DAYS + DAY_OF_YEAR ) * 86400 \
                    + ((__TIME__[0] - '0') * 10 + __TIME__[1] - '0') * 3600 \
                    + ((__TIME__[3] - '0') * 10 + __TIME__[4] - '0') * 60 \
                    + ((__TIME__[6] - '0') * 10 + __TIME__[7] - '0') )
#define  __DATE_TIME_UNIX__ ( __DATE_TIME_Y2K__ + Y2K_UNIX_EPOCH_DIFF )
#endif /* __DATE_TIME_H__ */