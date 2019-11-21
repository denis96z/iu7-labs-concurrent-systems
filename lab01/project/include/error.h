#ifndef LAB01_ERROR_H
#define LAB01_ERROR_H

typedef enum
{
    OK,
    ARG_ERR,
    FIO_ERR,
    FMT_ERR,
    MEM_ERR,
} error_t;

#define ERR_TO_RET_CODE(x) \
    -((error_t)(x) + 1)

#endif //LAB01_ERROR_H
