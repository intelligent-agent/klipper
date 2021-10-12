
#include "autoconf.h" // CONFIG_CLOCK_REF_FREQ
#include "board/armcm_boot.h" // armcm_main
#include "board/irq.h" // irq_disable
#include "command.h" // DECL_CONSTANT_STR
#include "internal.h" // enable_pclock
#include "sched.h" // sched_main
#include "gpio.h" // gpio_pwm_write

uint8_t flash_unlock(void){
    if((FLASH->CR & FLASH_CR_LOCK) != RESET){
      FLASH->KEYR = FLASH_KEY1;
      FLASH->KEYR = FLASH_KEY2;
      if((FLASH->CR & FLASH_CR_LOCK) != RESET)
        return 1;
    }
    return 0;
}

void flash_lock(void){
  FLASH->CR |= FLASH_CR_LOCK;
}

uint8_t flash_wait_for_finish(void){
  while(FLASH->SR & FLASH_SR_BSY);
  if(FLASH->SR & FLASH_SR_EOP){
    FLASH->SR = FLASH_SR_EOP;
  }
  if((FLASH->SR & FLASH_SR_WRPERR) || (FLASH->SR & FLASH_SR_PGERR)){
    return 1;
  }
  return 0;
}

void flash_save_word(uint16_t word, uint32_t addr){
  FLASH->CR |= FLASH_CR_PG;
  *(__IO uint16_t*) addr = word;
  flash_wait_for_finish();
}

uint16_t flash_read_word(uint32_t addr){
  return *(uint16_t*) addr;
}

void flash_erase_page(uint32_t addr){
  FLASH->CR |= FLASH_CR_PER;
  FLASH->AR = addr;
  FLASH->CR |= FLASH_CR_STRT;
  flash_wait_for_finish();
}

void flash_erase_position(void){
  uint32_t addr = 0x08003c00;
  irq_disable();
  serial_disable_tx_irq();
  flash_unlock();
  flash_wait_for_finish();
  flash_erase_page(addr);
  serial_enable_tx_irq();
  irq_enable();
  FLASH->CR &= ~FLASH_CR_PER;
  flash_lock();
}

void flash_save_position(struct position pos){
  uint32_t addr = 0x08003c00;
  flash_unlock();
  flash_wait_for_finish();
  flash_save_word(pos.x, addr+0);
  flash_save_word(pos.y, addr+2);
  flash_save_word(pos.z, addr+4);
  flash_save_word(pos.timestamp, addr+6);
  flash_save_word(pos.saved, addr+8);
  FLASH->CR &= ~FLASH_CR_PG;
  flash_lock();
}

struct position flash_read_position(void){
  uint32_t addr = 0x08003c00;
  struct position pos;
  pos.x = flash_read_word(addr);
  pos.y = flash_read_word(addr+2);
  pos.z = flash_read_word(addr+4);
  pos.timestamp = flash_read_word(addr+6);
  pos.saved = flash_read_word(addr+8);
  return pos;
}
