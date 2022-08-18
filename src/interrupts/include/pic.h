#ifndef PIC_H
#define PIC_H

void pic_init(void);
void pic_send_EOI(uint32_t);
void pic_disable(void);

#endif
