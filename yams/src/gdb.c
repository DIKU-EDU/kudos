/* yams -- Yet Another Machine Simulator
   Copyright (C) 2006 Juha Aatrokoski, Timo Lilja, Leena Salmela,
     Teemu Takanen, Aleksi Virtanen

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
   USA.

   $Id: gdb.c,v 1.3 2010/11/24 16:52:19 jaatroko Exp $

   A GDB remote serial protocol interface implementation to yams.

   This file is based on the public domain file 'sparc-stub.c'
   distributed with the GDB source code.

   See function process_gdb_loop() for a list of supported GDB protocol
   commands.
*/


#include "includes.h"

#include <string.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gdb.h"
#include "cpu.h"
#include "memory.h"
#include "simulator.h"
#include "hwconsole.h"
#include "async_input.h"

/* if debug_packets is true, all the incoming and outgoing packets
   are printed to yams stdout */
static int debug_packets = 0;

/*****************************************************
 * Packet handling routines (taken from sparc-stub.c)*
 *****************************************************/

/* BUFMAX defines the maximum number of characters in inbound/outbound
 * buffers at least NUMREGBYTES*2 are needed for register packets */
#define BUFMAX 2048

static const char hexchars[]="0123456789abcdef";

/* listening socket */
static int sock;
/* gdb socket */
static int sock_fd;

/* write a single character      */
static void putDebugChar(char c)
{
    ssize_t nbytes;
    do {
        nbytes = write(sock_fd, &c, 1);
    } while (nbytes == 0);

    if (nbytes < 0) {
        perror("getDebugChar: write");
        exit(EXIT_FAILURE);
    }
}

/* read and return a single char */
static int getDebugChar()
{
    uint8_t c;
    size_t nbytes = read(sock_fd, &c, 1);

    if (nbytes < 0) {
        perror("getDebugChar: read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0)
        return EOF;
    else
        return c;
}


/* Convert ch from a hex digit to an int */
static int hex(unsigned char ch)
{
  if (ch >= 'a' && ch <= 'f')
    return ch-'a'+10;
  if (ch >= '0' && ch <= '9')
    return ch-'0';
  if (ch >= 'A' && ch <= 'F')
    return ch-'A'+10;
  return -1;
}

static char remcomInBuffer[BUFMAX];
static char remcomOutBuffer[BUFMAX];

/* scan for the sequence $<data>#<checksum>     */
static unsigned char *getpacket(void)
{
  char *buffer = &remcomInBuffer[0];
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  char ch;

  while (1)
    {
      /* wait around for the start character, ignore all other characters */
      while ((ch = getDebugChar ()) != '$' && ch != EOF)
	;

      if (ch == EOF)
          return NULL;
retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* now, read until a # or end of buffer is found */
      while (count < BUFMAX)
	{
	  ch = getDebugChar ();
          if (ch == '$')
            goto retry;
	  if (ch == '#')
	    break;
	  checksum = checksum + ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;

      if (ch == '#')
	{
	  ch = getDebugChar ();
	  xmitcsum = hex (ch) << 4;
	  ch = getDebugChar ();
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    {
	      putDebugChar ('-');	/* failed checksum */
	    }
	  else
	    {
	      putDebugChar ('+');	/* successful transfer */

	      /* if a sequence char is present, reply the sequence ID */
	      if (buffer[2] == ':')
		{
		  putDebugChar (buffer[0]);
		  putDebugChar (buffer[1]);

		  return (unsigned char*)&buffer[3];
		}

	      return (unsigned char*)&buffer[0];
	    }
	}
    }
}

/* send the packet in buffer.  */
static void putpacket(unsigned char *buffer)
{
  unsigned char checksum;
  int count;
  unsigned char ch;

  /*  $<packet info>#<checksum>. */
  do
    {
      putDebugChar('$');
      checksum = 0;
      count = 0;

      while ((ch = buffer[count]))
	{
	  putDebugChar(ch);
	  checksum += ch;
	  count += 1;
	}

      putDebugChar('#');
      putDebugChar(hexchars[checksum >> 4]);
      putDebugChar(hexchars[checksum & 0xf]);

    }
  while (getDebugChar() != '+');
}

/*
 * While we find nice hex chars, build an int.
 * Return number of chars processed.
 */
static int hexToInt(char **ptr, int *intValue)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr)
    {
      hexValue = hex(**ptr);
      if (hexValue < 0)
	break;

      *intValue = (*intValue << 4) | hexValue;
      numChars ++;

      (*ptr)++;
    }

  return (numChars);
}

static int hexToNum(char **ptr, int *intValue, int length)
{
  int numChars = 0;
  int hexValue;

  *intValue = 0;

  while (**ptr && length > 0)
    {
      hexValue = hex(**ptr);
      if (hexValue < 0)
	break;

      *intValue = (*intValue << 4) | hexValue;
      numChars ++;

      (*ptr)++;
      length--;
    }

  return (numChars);
}

/*************************************************
 * Interface routines to the yams MIPS simulator *
 *************************************************/
static uint32_t read_register(int cpunum, int regnum)
{
    cpu_t *cpu = hardware->cpus[cpunum];

    if (regnum < 32)
        return READ_REG(cpu, regnum); /* general purpose registers */
    else if (regnum == 32)
        return READ_CP0_REG(cpu->cp0, 12);  /* sr */
    else if (regnum == 33)
        return READ_REG(cpu, 34); /* lo */
    else if (regnum == 34)
        return READ_REG(cpu, 33); /* hi */
    else if (regnum == 35)
        return READ_CP0_REG(cpu->cp0, 8); /* bad */
    else if (regnum == 36)
        return READ_CP0_REG(cpu->cp0, 13); /* cause */
    else if (regnum == 37)
        return READ_REG(cpu, 32); /* pc */
    else
        return 0x00000000; /* fp registers zero */
}

static void read_registers(int cpunum, char *result)
{
    int i;

    /* register packet format:
       32 general-purpose; sr; lo; hi; bad; cause; pc;
       32 floating-point registers; fsr; fir; fp.
       32 + 6 + 32 + 3 = 73 in total
    */
    for (i = 0; i < 73; i++)
        result += sprintf(result, "%08x", read_register(cpunum, i));
}

static void write_register(int cpunum, int regnum, uint32_t new_val)
{
    cpu_t *cpu = hardware->cpus[cpunum];

    if (regnum < 32)
        WRITE_REG(cpu, regnum, new_val); /* general purpose registers */
    else if (regnum == 32)
        WRITE_CP0_REG(cpu->cp0, 12, new_val);  /* sr */
    else if (regnum == 33)
        WRITE_REG(cpu, 34, new_val); /* lo */
    else if (regnum == 34)
        WRITE_REG(cpu, 33, new_val); /* hi */
    else if (regnum == 35)
        WRITE_CP0_REG(cpu->cp0, 8, new_val); /* bad */
    else if (regnum == 36)
        WRITE_CP0_REG(cpu->cp0, 13, new_val); /* cause */
    else if (regnum == 37)
        WRITE_REG(cpu, 32, new_val); /* pc */
    /* ignore fp registers */
}


static void write_registers(int cpunum, char *new_values)
{
    uint32_t new_val;
    int i;

    for (i = 0; i < 73; i++) {
        hexToNum(&new_values, (int*)&new_val, 4);
        write_register(cpunum, i, new_val);
    }
}

static void set_pc(int cpunum, uint32_t pc)
{
    cpu_t *cpu = hardware->cpus[cpunum];
    WRITE_REG(cpu, 32, pc);
}

static int read_mem(uint32_t addr, uint32_t length, char *result)
{
    exception_t exc;
    uint32_t save_status;
    uint8_t buf;
    uint32_t i;
    /* This kludge assures that mem_read thinks the CP0
     * is in kernel mode
     */
    save_status = hardware->cpus[0]->cp0->registers[Status];
    hardware->cpus[0]->cp0->registers[Status] = 6;
    for (i = 0; i < length; i++) {
        exc = mem_read(hardware->memory, addr + i, &buf,
                       1, hardware->cpus[0]);
        result += sprintf(result, "%02x", buf);
    }
    hardware->cpus[0]->cp0->registers[Status] = save_status;

    /* XXX Exception handling? */
    return 0;
}


static int write_mem(uint32_t addr, uint32_t length, char *new_value)
{
    exception_t exc;
    uint32_t save_status;
    int mem_byte;
    int i;
    /* This kludge assures that mem_write thinks the CP0
     * is in kernel mode
     */
    save_status = hardware->cpus[0]->cp0->registers[Status];
    hardware->cpus[0]->cp0->registers[Status] = 6;
    for (i = 0; i < length; i++) {
        hexToNum(&new_value, &mem_byte, 2);
        exc = mem_write(hardware->memory, addr + i, &mem_byte,
                        1, hardware->cpus[0]);
        }
    hardware->cpus[0]->cp0->registers[Status] = save_status;

    /* XXX Exception handling? */
    return 0;
}


/*********************************************
 * GDB remote serial protocol implementation *
 *********************************************/

/* return value: 0 if the simulator is to be killed,
 *               1 if the simulator is to be continued.
 */
static int process_gdb_loop(void)
{
  int addr;
  int length;
  int cpu_id;
  int step_cpu, other_cpu = 0;
  char *ptr;
  char type;
  int regnum;
  uint32_t val;

  regnum = regnum;
  step_cpu = other_cpu = 0;

  /* if the hardware is running, we dropped here because the user has
   * hit break in gdb, so we send a signal to GDB indicating that */
  if (hardware->running == 1) {
      remcomOutBuffer[0] = 'S';
      remcomOutBuffer[1] = '0';
      remcomOutBuffer[2] = '5';
      remcomOutBuffer[3] = 0;
      putpacket((unsigned char *)remcomOutBuffer);
  }

  while (1)
    {
      remcomOutBuffer[0] = 0;

      ptr = (char*)getpacket();
      if (ptr == NULL) {
          /* we didn't receive a valid packet, assume that
             the connection has been terminated */
          gdb_interface_close();

          return 1;
      }

      if (debug_packets) printf("from gdb:%s\n", ptr);
      switch (*ptr++) {
      case '?': /* `?' -- last signal */
          remcomOutBuffer[0] = 'S';
          remcomOutBuffer[1] = '0';
          remcomOutBuffer[2] = '1';
          remcomOutBuffer[3] = 0;
          break;
      case 'c':    /* cAA..AA    Continue at address AA..AA(optional) */
          if (hexToInt(&ptr, &addr))
              set_pc(step_cpu, addr);
          hardware->running = 1;
          return 1;
          break;
      case 'd': /* `d' -- toggle debug *(deprecated)* */
          debug_packets = (debug_packets + 1) % 2;
          break;
      case 'g':		/* return the value of the CPU registers */
          read_registers(other_cpu, remcomOutBuffer);
          break;
      case 'G':	   /* set the value of the CPU registers - return OK */
          write_registers(other_cpu, ptr);
          strcpy(remcomOutBuffer,"OK");
	  break;
      case 'H': /* `H'CT... -- set thread */
          type = *ptr++;
          if (hexToInt(&ptr, &cpu_id)) {
              if (cpu_id == -1 || cpu_id == 0) /* XXX all threads */
                  cpu_id = 1;
              if (type == 'c') {
                  step_cpu = cpu_id - 1; /* minus one because
                                            gdb threats start from 1
                                            and yams cpu's from 0. */
                  strcpy(remcomOutBuffer, "OK");
              } else if (type == 'g') {
                  other_cpu = cpu_id - 1; /* same here */
                  strcpy(remcomOutBuffer, "OK");
              } else
                  strcpy(remcomOutBuffer, "E01");
          } else
              strcpy(remcomOutBuffer, "E01");
          break;
      case 'k' : 	  /* kill the program */
          return 0;
          break;
      case 'm':	  /* mAA..AA,LLLL  Read LLLL bytes at address AA..AA */
          if (hexToInt(&ptr,&addr) &&*ptr++==','&& hexToInt(&ptr,&length)) {
              if (read_mem(addr, length, remcomOutBuffer))
                  strcpy(remcomOutBuffer, "E03");
          } else
              strcpy(remcomOutBuffer, "E01");
          break;
      case 'M': /* MAA..AA,LLLL: Write LLLL bytes at address AA.AA */
          if (hexToInt(&ptr, &addr) && *ptr++ == ','
              && hexToInt(&ptr, &length) && *ptr++ == ':') {
              if (!write_mem(addr, length, ptr))
                  strcpy(remcomOutBuffer, "OK");
              else
                  strcpy(remcomOutBuffer, "E03");
          }
          else
              strcpy(remcomOutBuffer, "E02");
          break;
      case 'p': /* `p'HEX NUMBER OF REGISTER -- read register packet */
          if (hexToInt(&ptr, &regnum) && regnum <= 73)
              sprintf(remcomOutBuffer, "%08x",
                      read_register(other_cpu, regnum));
          else
              sprintf(remcomOutBuffer, "E01");
          break;
      case 'P': /* `P'N...`='R... -- write register */
          if (hexToInt(&ptr, (int*)&regnum) && *ptr++=='=' && hexToInt(&ptr, (int*)&val)) {
              write_register(other_cpu, regnum, val);
              sprintf(remcomOutBuffer, "OK");
          } else
              sprintf(remcomOutBuffer, "E01");
      case 'q': /* `q'QUERY -- general query */
          if (!strcmp(ptr, "fThreadInfo")) {
              int i;
              char *ptr = remcomOutBuffer;
              ptr += sprintf(ptr, "m01");
              if (hardware->num_cpus > 1)
                  for (i = 1; i < hardware->num_cpus; i++)
                      ptr += sprintf(ptr, ",%02x", i + 1);
              sprintf(ptr, "l");
          }
          break;
      case 's': /* `s'ADDR -- step */
          command_step(1);
          sprintf(remcomOutBuffer, "S01");
          break;
      case 'T': /* `T'XX -- thread alive */
          if (hexToInt(&ptr, &cpu_id) && --cpu_id < hardware->num_cpus)
              strcpy(remcomOutBuffer, "OK");
          else
              strcpy(remcomOutBuffer, "E01");
          break;
      case 'z':   /* remove breakpoint: `Z'TYPE`,'ADDR`,'LENGTH */
          type = *ptr++;
          if (*ptr++== ',' && hexToInt(&ptr, &addr)
              && *ptr++ == ',' && hexToInt(&ptr, &length)) {
              if (type == '1') { /* hardware breakpoint */
                  command_breakpoint(0xFFFFFFFF);
                  strcpy(remcomOutBuffer, "OK");
              } else /* all others are unsupported */
                  strcpy(remcomOutBuffer, "E01");
          } else
              strcpy(remcomOutBuffer, "E02");
          break;
      case 'Z':  /* insert breakpoint: `Z'TYPE`,'ADDR`,'LENGTH */
          type = *ptr++;
          if (*ptr++== ',' && hexToInt(&ptr, &addr)
              && *ptr++ == ',' && hexToInt(&ptr, &length)) {
              if (type == '1') { /* hardware breakpoint */
                  command_breakpoint(addr);
                  strcpy(remcomOutBuffer, "OK");
              } else /* all others are unsupported */
                  strcpy(remcomOutBuffer, "E01");

          } else
              strcpy(remcomOutBuffer, "E02");
          break;
      default:
          break;
      }			/* switch */

      /* reply to the request */
      putpacket((unsigned char *)remcomOutBuffer);
      if (debug_packets) printf("to gdb: %s\n", remcomOutBuffer);
    }

}

static int gdb_interface_connected = 0;

/********************
 * Public interface *
 ********************/
void gdb_interface_open(uint16_t port)
{

    struct sockaddr_in name;
    struct sockaddr_in addr;
    size_t addr_size;
    int op;

    printf("gdb interface listening on port %d\n", port);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 0) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    addr_size = sizeof(addr);
    if ((sock_fd = accept(sock, (struct sockaddr *)&addr,  (socklen_t*)&addr_size)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("Connect from host %s, port %d.\n", inet_ntoa(addr.sin_addr),
           ntohs(addr.sin_port));


    if (async_input_register_fd(sock_fd)) {
        fprintf(stderr, "gdb: error registering fd to async input\n");
        exit(1);
    }

    gdb_interface_connected = 1;
}

/* return 0 if the simulation should be stopped, 1 if it should
 * continue */
int gdb_interface_check_and_run(void)
{
   if (gdb_interface_connected &&
       (async_input_check_fd(sock_fd) || (hardware->running == 0))) {
        return process_gdb_loop();
    }

   /* we return 1 here to indicate that the simulation should go on */
   return 1;
}



void gdb_interface_close()
{
    if (close(sock_fd) < 0) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    if (close(sock) < 0) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}
