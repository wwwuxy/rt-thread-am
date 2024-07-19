#include <am.h>
#include <klib.h>
#include <rtthread.h>



static Context* ev_handler(Event e, Context *c) {
  Context **rt_from,**rt_to;
  Context ***from_to = (Context ***)rt_thread_self()->user_data;
  rt_from = from_to[0];
  rt_to = from_to[1];

  switch (e.event) {
    case EVENT_YIELD: 
      if(rt_from){
        *rt_from = c;
      }
      c = *rt_to;
      break;
    case EVENT_IRQ_TIMER: 
      break;
    default: printf("Unhandled event ID = %d\n", e.event); 
             assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_ubase_t current = rt_thread_self()->user_data;
  rt_ubase_t from_to[2] = {from, to};
  rt_thread_self()->user_data = (rt_ubase_t)from_to;
  yield();
  rt_thread_self()->user_data = current;
}


void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_hw_context_switch(0,to);
}



void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

void entry_wrapper(void *args) {  //即kcontext中的entry函数
  void **p = args;
  void (*tentry)(void *) = p[0];  //tentry是线程入口函数,参数为void*类型-->parameter
  void *parameter = p[1];
  void (*texit)(void) = p[2];
  tentry(parameter);
  texit();
}

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  rt_uint8_t *addr = stack_addr;
  uintptr_t addr_i = (uintptr_t)addr; // 将指针转换为整数
  uintptr_t aligned_addr_value = RT_ALIGN(addr_i, sizeof(uintptr_t)); // 对齐到 sizeof(uintptr_t)
  rt_uint8_t *aligned_addr = (rt_uint8_t *)aligned_addr_value;

//开栈，将三个参数放入栈中
  aligned_addr -= sizeof(uintptr_t) * 3;
  void **args = (void **)aligned_addr;
  args[0] = tentry;
  args[1] = parameter;
  args[2] = texit;
  
  rt_uint8_t *stack_start = aligned_addr - sizeof(Context);   //不确定要不要 - sizeof(uintptr_t)

  return (rt_uint8_t *)kcontext((Area){stack_start, aligned_addr}, entry_wrapper, args);
}
