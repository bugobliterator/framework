#include "platform_stm32f302x8.h"
#include <hal.h>
#include <string.h>

#define VAL_GPIOA_MODER 0xA8000000
#define VAL_GPIOB_MODER 0x00000280
#define VAL_GPIOC_MODER 0x00000000
#define VAL_GPIOD_MODER 0x00000000
#define VAL_GPIOE_MODER 0x00000000
#define VAL_GPIOF_MODER 0x00000000
#define VAL_GPIOG_MODER 0x00000000
#define VAL_GPIOH_MODER 0x00000000

#define VAL_GPIOA_OTYPER 0x00000000
#define VAL_GPIOB_OTYPER 0x00000000
#define VAL_GPIOC_OTYPER 0x00000000
#define VAL_GPIOD_OTYPER 0x00000000
#define VAL_GPIOE_OTYPER 0x00000000
#define VAL_GPIOF_OTYPER 0x00000000
#define VAL_GPIOG_OTYPER 0x00000000
#define VAL_GPIOH_OTYPER 0x00000000

#define VAL_GPIOA_OSPEEDR 0xC0000000
#define VAL_GPIOB_OSPEEDR 0x000000C0
#define VAL_GPIOC_OSPEEDR 0x00000000
#define VAL_GPIOD_OSPEEDR 0x00000000
#define VAL_GPIOE_OSPEEDR 0x00000000
#define VAL_GPIOF_OSPEEDR 0x00000000
#define VAL_GPIOG_OSPEEDR 0x00000000
#define VAL_GPIOH_OSPEEDR 0x00000000

#define VAL_GPIOA_PUPDR 0x64000000
#define VAL_GPIOB_PUPDR 0x00000100
#define VAL_GPIOC_PUPDR 0x0C000000
#define VAL_GPIOD_PUPDR 0x0C000000
#define VAL_GPIOE_PUPDR 0x0C000000
#define VAL_GPIOF_PUPDR 0x0C000000
#define VAL_GPIOG_PUPDR 0x0C000000
#define VAL_GPIOH_PUPDR 0x0C000000

#define VAL_GPIOA_ODR 0x00000000
#define VAL_GPIOB_ODR 0x00000000
#define VAL_GPIOC_ODR 0x00000000
#define VAL_GPIOD_ODR 0x00000000
#define VAL_GPIOE_ODR 0x00000000
#define VAL_GPIOF_ODR 0x00000000
#define VAL_GPIOG_ODR 0x00000000
#define VAL_GPIOH_ODR 0x00000000

#define VAL_GPIOA_AFRL 0x00000000
#define VAL_GPIOB_AFRL 0x00000000
#define VAL_GPIOC_AFRL 0x00000000
#define VAL_GPIOD_AFRL 0x00000000
#define VAL_GPIOE_AFRL 0x00000000
#define VAL_GPIOF_AFRL 0x00000000
#define VAL_GPIOG_AFRL 0x00000000
#define VAL_GPIOH_AFRL 0x00000000

#define VAL_GPIOA_AFRH 0x00000000
#define VAL_GPIOB_AFRH 0x00000000
#define VAL_GPIOC_AFRH 0x00000000
#define VAL_GPIOD_AFRH 0x00000000
#define VAL_GPIOE_AFRH 0x00000000
#define VAL_GPIOF_AFRH 0x00000000
#define VAL_GPIOG_AFRH 0x00000000
#define VAL_GPIOH_AFRH 0x00000000

#if HAL_USE_PAL || defined(__DOXYGEN__)
/**
 * @brief   PAL setup.
 * @details Digital I/O ports static configuration as defined in @p board.h.
 *          This variable is used by the HAL when initializing the PAL driver.
 */

const PALConfig pal_default_config = {
#if STM32_HAS_GPIOA
  {VAL_GPIOA_MODER, VAL_GPIOA_OTYPER, VAL_GPIOA_OSPEEDR, VAL_GPIOA_PUPDR,
   VAL_GPIOA_ODR,   VAL_GPIOA_AFRL,   VAL_GPIOA_AFRH},
#endif
#if STM32_HAS_GPIOB
  {VAL_GPIOB_MODER, VAL_GPIOB_OTYPER, VAL_GPIOB_OSPEEDR, VAL_GPIOB_PUPDR,
   VAL_GPIOB_ODR,   VAL_GPIOB_AFRL,   VAL_GPIOB_AFRH},
#endif
#if STM32_HAS_GPIOC
  {VAL_GPIOC_MODER, VAL_GPIOC_OTYPER, VAL_GPIOC_OSPEEDR, VAL_GPIOC_PUPDR,
   VAL_GPIOC_ODR,   VAL_GPIOC_AFRL,   VAL_GPIOC_AFRH},
#endif
#if STM32_HAS_GPIOD
  {VAL_GPIOD_MODER, VAL_GPIOD_OTYPER, VAL_GPIOD_OSPEEDR, VAL_GPIOD_PUPDR,
   VAL_GPIOD_ODR,   VAL_GPIOD_AFRL,   VAL_GPIOD_AFRH},
#endif
#if STM32_HAS_GPIOE
  {VAL_GPIOE_MODER, VAL_GPIOE_OTYPER, VAL_GPIOE_OSPEEDR, VAL_GPIOE_PUPDR,
   VAL_GPIOE_ODR,   VAL_GPIOE_AFRL,   VAL_GPIOE_AFRH},
#endif
#if STM32_HAS_GPIOF
  {VAL_GPIOF_MODER, VAL_GPIOF_OTYPER, VAL_GPIOF_OSPEEDR, VAL_GPIOF_PUPDR,
   VAL_GPIOF_ODR,   VAL_GPIOF_AFRL,   VAL_GPIOF_AFRH},
#endif
#if STM32_HAS_GPIOG
  {VAL_GPIOG_MODER, VAL_GPIOG_OTYPER, VAL_GPIOG_OSPEEDR, VAL_GPIOG_PUPDR,
   VAL_GPIOG_ODR,   VAL_GPIOG_AFRL,   VAL_GPIOG_AFRH},
#endif
#if STM32_HAS_GPIOH
  {VAL_GPIOH_MODER, VAL_GPIOH_OTYPER, VAL_GPIOH_OSPEEDR, VAL_GPIOH_PUPDR,
   VAL_GPIOH_ODR,   VAL_GPIOH_AFRL,   VAL_GPIOH_AFRH},
#endif
#if STM32_HAS_GPIOI
  {VAL_GPIOI_MODER, VAL_GPIOI_OTYPER, VAL_GPIOI_OSPEEDR, VAL_GPIOI_PUPDR,
   VAL_GPIOI_ODR,   VAL_GPIOI_AFRL,   VAL_GPIOI_AFRH}
#endif
};
#endif

/**
 * @brief   Early initialization code.
 * @details This initialization must be performed just after stack setup
 *          and before any other initialization.
 */
void __early_init(void) {
    stm32_clock_init();
}

void board_get_unique_id(uint8_t* buf, uint8_t len) {
    uint32_t unique_id_uint32[3];
    unique_id_uint32[0] = ((uint32_t*)0x1FFFF7AC)[2];
    unique_id_uint32[1] = ((uint32_t*)0x1FFFF7AC)[1];
    unique_id_uint32[2] = ((uint32_t*)0x1FFFF7AC)[0];

    if (len>12) {
        memset(buf, 0, len);
        memcpy(buf, unique_id_uint32, 12);
    } else {
        memcpy(buf, unique_id_uint32, len);
    }
}
