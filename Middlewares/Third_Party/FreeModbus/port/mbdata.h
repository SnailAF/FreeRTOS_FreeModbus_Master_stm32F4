/**
  ******************************************************************************
  * @file    app_configure.h
  * @author  
  * @version 
  * @date    2021.8.18
  * @brief   工程模块裁剪用配置文件
  ******************************************************************************
  * @attention
  * 可添加程序说明及注意事项：
  * 1.
  * 2.
  ******************************************************************************
  */ 
	
#ifndef  PORT_MODBUS_DATA_H_
#define  PORT_MODBUS_DATA_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
#define MB_ASSERT_ENABLE 1
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
#if MB_ASSERT_ENABLE
#define MB_ASSERT(EXPR)                                                 \
if (!(EXPR)){                                                           \
	printf( "has assert failed at %s:%d.", __FILE__, __LINE__); \
	while (1);}
#else
    #define MB_ASSERT(EXPR)                    ((void)0);
#endif

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
/**
 * UTILS FUNCTIONS
 **/

#define MODBUS_GET_HIGH_BYTE(data) (((data) >> 8) & 0xFF)
#define MODBUS_GET_LOW_BYTE(data) ((data) & 0xFF)
#define MODBUS_GET_INT64_FROM_INT16(tab_int16, index) \
    (((int64_t)tab_int16[(index)    ] << 48) | \
     ((int64_t)tab_int16[(index) + 1] << 32) | \
     ((int64_t)tab_int16[(index) + 2] << 16) | \
      (int64_t)tab_int16[(index) + 3])
#define MODBUS_GET_INT32_FROM_INT16(tab_int16, index) \
    (((int32_t)tab_int16[(index)    ] << 16) | \
      (int32_t)tab_int16[(index) + 1])
#define MODBUS_GET_INT16_FROM_INT8(tab_int8, index) \
    (((int16_t)tab_int8[(index)    ] << 8) | \
      (int16_t)tab_int8[(index) + 1])
#define MODBUS_SET_INT16_TO_INT8(tab_int8, index, value) \
    do { \
        ((int8_t*)(tab_int8))[(index)    ] = (int8_t)((value) >> 8);  \
        ((int8_t*)(tab_int8))[(index) + 1] = (int8_t)(value); \
    } while (0)
#define MODBUS_SET_INT32_TO_INT16(tab_int16, index, value) \
    do { \
        ((int16_t*)(tab_int16))[(index)    ] = (int16_t)((value) >> 16); \
        ((int16_t*)(tab_int16))[(index) + 1] = (int16_t)(value); \
    } while (0)
#define MODBUS_SET_INT64_TO_INT16(tab_int16, index, value) \
    do { \
        ((int16_t*)(tab_int16))[(index)    ] = (int16_t)((value) >> 48); \
        ((int16_t*)(tab_int16))[(index) + 1] = (int16_t)((value) >> 32); \
        ((int16_t*)(tab_int16))[(index) + 2] = (int16_t)((value) >> 16); \
        ((int16_t*)(tab_int16))[(index) + 3] = (int16_t)(value); \
    } while (0)

void modbus_set_bits_from_byte(uint8_t *dest, int idx, const uint8_t value);
void modbus_set_bits_from_bytes(uint8_t *dest, int idx, unsigned int nb_bits,
                                       const uint8_t *tab_byte);
uint8_t modbus_get_byte_from_bits(const uint8_t *src, int idx, unsigned int nb_bits);
float modbus_get_float(const uint16_t *src);
float modbus_get_float_abcd(const uint16_t *src);
float modbus_get_float_dcba(const uint16_t *src);
float modbus_get_float_badc(const uint16_t *src);
float modbus_get_float_cdab(const uint16_t *src);

void modbus_set_float(float f, uint16_t *dest);
void modbus_set_float_abcd(float f, uint16_t *dest);
void modbus_set_float_dcba(float f, uint16_t *dest);
void modbus_set_float_badc(float f, uint16_t *dest);
void modbus_set_float_cdab(float f, uint16_t *dest);
//int16_t modbus_get_int16_form_sint16(uint16_t data);


//hold reg fun
void mb_hd_fset (float f,uint16_t add);
void mb_hd_fset_abcd (float f,uint16_t add);
void mb_hd_fset_dcba (float f,uint16_t add);
void mb_hd_fset_badc (float f,uint16_t add);
void mb_hd_fset_cdab (float f,uint16_t add);
void mb_hd_s64set (int64_t d,uint16_t add);
void mb_hd_s32set (int32_t d,uint16_t add);
void mb_hd_s16set (int16_t d,uint16_t add);
float mb_hd_fget (uint16_t add);
float mb_hd_fget_abcd (uint16_t add);
float mb_hd_fget_dcba (uint16_t add);
float mb_hd_fget_badc (uint16_t add);
float mb_hd_fget_cdab (uint16_t add);
int64_t mb_hd_s64get (uint16_t add);
int32_t mb_hd_s32get (uint16_t add);
int16_t mb_hd_s16get (uint16_t add);

// input reg
void mb_in_fset (float f,uint16_t add);
void mb_in_fset_abcd (float f,uint16_t add);
void mb_in_fset_dcba (float f,uint16_t add);
void mb_in_fset_badc (float f,uint16_t add);
void mb_in_fset_cdab (float f,uint16_t add);
void mb_in_s64set (int64_t d,uint16_t add);
void mb_in_s32set (int32_t d,uint16_t add);
void mb_in_s16set (int16_t d,uint16_t add);
float mb_in_fget (uint16_t add);
float mb_in_fget_abcd (uint16_t add);
float mb_in_fget_dcba (uint16_t add);
float mb_in_fget_badc (uint16_t add);
float mb_in_fget_cdab (uint16_t add);
int64_t mb_in_s64get (uint16_t add);
int32_t mb_in_s32get (uint16_t add);
int16_t mb_in_s16get(uint16_t add);
// coil reg
void mb_co_set(uint8_t d,uint16_t add);
uint8_t mb_co_get(uint16_t add);

// discrete reg
void mb_di_set(uint8_t d,uint16_t add);
uint8_t mb_di_get(uint16_t add);

#ifdef __cplusplus
}
#endif

#endif /*  PORT_MODBUS_DATA_H_ */
