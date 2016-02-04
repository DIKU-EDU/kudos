#ifndef YAMS_GDB_H
#define YAMS_GDB_H

void gdb_interface_open(uint16_t port);
int gdb_interface_check_and_run(void);
void gdb_interface_close(void);

#endif /* YAMS_GDB_H */
