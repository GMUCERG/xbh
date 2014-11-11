#ifndef __UTILS_H__
#define __UTILS_H__

/* 
 * http://stackoverflow.com/questions/9907160/how-to-convert-enum-names-to-string-in-c 
 */
#define GEN_ENUM(ENUM) ENUM,
#define GEN_STR(STRING) #STRING,

/*
 * #define FOREACH_FRUIT(FRUIT) \
 *      FRUIT(apple)   \
 *      FRUIT(orange)  \
 *      FRUIT(grape)   \
 *      FRUIT(banana)  
 */
/*
 * enum FRUIT_ENUM {
 *      FOREACH_FRUIT(GEN_ENUM)
 * };
 */
/*
 * static const char *FRUIT_STRING[] = {
 *      FOREACH_FRUIT(GEN_STR)
 * };
 */

#endif
