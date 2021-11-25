// Copyright (C) 2020  Elias Bakken <elias@iagent.no>
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "gpio.h"
#include "util.h"

#define BANK(x) (x/32)
#define PIN(x) (x%32)
#define CFG_REG(x) ((x/8)*4)
#define CFG_OFF(x) ((x%8)*4)
#define PULLUP_REG(x) 0x1C + ((x/16)*4)
#define PULLUP_OFF(x) ((x%16)*2)

volatile uint32_t data_regs[8];

struct gpio_mux gpio_mux_setup(uint8_t pin, enum pin_func func){
  uint8_t bank = BANK(pin);
  uint8_t p = PIN(pin);
  uint32_t data_reg = PIO_BASE + bank*0x24 + 0x10;
  uint32_t cfg_reg = PIO_BASE + bank*0x24 + CFG_REG(p);
  uint8_t cfg_off = CFG_OFF(p);

  if(bank == 0){ // Handle R_PIO
    data_reg = R_PIO_BASE + 0x10;
    cfg_reg = R_PIO_BASE + CFG_REG(p);
  }

  uint32_t curr_val = read_reg(cfg_reg) & ~(0xF<<cfg_off);
  write_reg(cfg_reg, curr_val | func<<cfg_off);

  struct gpio_mux ret = {
    .pin = p,
    .reg = data_reg,
    .bank = bank
  };
  return ret;
}

struct gpio_out gpio_out_setup(uint8_t pin, uint8_t val){

  struct gpio_mux mux = gpio_mux_setup(pin, PIO_OUTPUT);
  struct gpio_out ret = {
    .pin = mux.pin,
    .reg = mux.reg,
    .bank = mux.bank,
  };
  data_regs[ret.bank] = read_reg(ret.reg);
  gpio_out_write(ret, val);

  return ret;
}

void gpio_out_write(struct gpio_out pin, uint8_t val){
  data_regs[pin.bank] &= ~(1<<pin.pin);
  data_regs[pin.bank] |= ((!!val)<<pin.pin);
  write_reg(pin.reg, data_regs[pin.bank]);
}

void gpio_out_reset(struct gpio_out pin, uint8_t val){
  gpio_out_setup(pin.pin, val);
}

uint8_t gpio_in_read(struct gpio_in pin){
  data_regs[pin.bank] = read_reg(pin.reg);
  return !!(data_regs[pin.bank] & (1<<pin.pin));
}

// TODO: support for pull-up
struct gpio_in gpio_in_setup(uint8_t pin, int8_t pull_up){
  struct gpio_mux mux = gpio_mux_setup(pin, PIO_INPUT);

  uint32_t pullup_reg = PIO_BASE + mux.bank*0x24 + PULLUP_REG(mux.pin);
  uint8_t  pullup_off = PULLUP_OFF(mux.pin);

  if(mux.bank == 0){ // Handle R_PIO
    pullup_reg = R_PIO_BASE + PULLUP_REG(mux.pin);
  }

  write_reg(pullup_reg, pull_up<<pullup_off);

  struct gpio_in in = {
    .pin = mux.pin,
    .reg = mux.reg,
    .bank = mux.bank
  };
  data_regs[mux.bank] = read_reg(mux.reg);
  return in;
}

void gpio_in_reset(struct gpio_in pin, int8_t pull_up){
  int8_t p = pin.bank * 32 + pin.pin;
  gpio_in_setup(p, pull_up);
}