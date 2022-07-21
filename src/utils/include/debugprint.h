#ifndef	DEBUGPRINT_H
#define	DEBUGPRINT_H

extern struct VGA_Target *debugOutput;

void setDebugOutputSource(struct VGA_Target *target);
void print(const char *string);
void printf(const char *format, ...);
void putg(char g);

#endif
