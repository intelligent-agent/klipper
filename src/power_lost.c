#include "basecmd.h" // oid_alloc
#include "board/gpio.h" // struct gpio_in
#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_read_time
#include "command.h" // DECL_COMMAND
#include "sched.h" // DECL_TASK


struct gpio_out g;
void toggle_pin(void){
  gpio_out_toggle_noirq(g);
}

void setup_toggle(void){
  g = gpio_out_setup(16+7, 0);
}

struct position pos;

void
command_set_position(uint32_t *args)
{
    pos.x = args[1];
    pos.y = args[2];
    pos.z = args[3];
    pos.timestamp = args[4];
}
DECL_COMMAND(command_set_position, "set_position oid=%c x=%u y=%u z=%u ts=%u");

void
command_clear_position(uint32_t *args)
{
  setup_toggle();
  toggle_pin();
  flash_erase_position();
  toggle_pin();
}
DECL_COMMAND(command_clear_position, "clear_position oid=%c");

void
command_get_position(uint32_t *args)
{
    uint8_t oid = args[0];
    struct position old = flash_read_position();
    sendf("position_state oid=%c x=%u y=%u z=%u", oid, old.x, old.y, old.z);
}
DECL_COMMAND(command_get_position, "get_position oid=%c");


void save_position(void){
  setup_toggle();
  toggle_pin();
  struct position saved_pos = flash_read_position();
  if(saved_pos.saved == 1)
    return;

  pos.saved = 1;
  flash_save_position(pos);
  toggle_pin();
}
DECL_SHUTDOWN(save_position);
