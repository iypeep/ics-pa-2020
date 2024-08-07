#ifndef __AMUNIT_H__
#define __AMUNIT_H__

#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#define IOE ({ ioe_init();  })//好像不用在nemu/trm.c/_trm_init()中添加ioe_init();
#define CTE(h) ({ Context *h(Event, Context *); cte_init(h); })
#define VME(f1, f2) ({ void *f1(int); void f2(void *); vme_init(f1, f2); })
#define MPE ({ mpe_init(entry); })

extern void (*entry)();

#define CASE(id, entry_, ...) \
  case id: { \
    void entry_(); \
    entry = entry_; \
    __VA_ARGS__; \
    entry_(); \
    break; \
  }

#endif
