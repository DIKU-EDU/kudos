%name-prefix "cfg_"

%{
#include "includes.h"
#include "cfg.h"

extern int cfg_lineno;

int cfg_lex();

void yyerror (s)  /* Called by yyparse on error */
char *s;
{
    fprintf(stderr, "Error in configuration file: %s at line %d\n",
     s, cfg_lineno);
     exit(1);
}
%}

%union {
	uint32_t intvalue;
	char *stringvalue;
}

%token SECTION
%token ENDSECTION
%token SIMULATOR
%token DISK
%token TTY
%token NIC
%token PLUGIN

%token CLOCKSPEED
%token MEMORY
%token CPUS
%token CPUIRQ
%token BIGENDIAN
%token LITTLEENDIAN

%token VENDOR
%token IRQ
%token SECTORSIZE
%token CYLINDERS
%token NUMSECTORS
%token ROTTIME
%token SEEKTIME
%token FILENAME

%token SOCKET
%token SENDDELAY

%token MTU
%token MAC
%token RELIABILITY
%token DMADELAY

%token OPTIONS
%token ASYNC

%token UNIXSOCKET
%token TCPHOST
%token UDPHOST
%token PORT
%token SOCKETLISTEN
%token <intvalue> INTEGER32
%token <stringvalue> STRING
%token ERROR

%%
config: options config {}
      | options {}
;

options: SECTION SIMULATOR { cfg_simoptions_start(); }
            simoptions
         ENDSECTION        { cfg_simoptions_end(); }
       | SECTION DISK      { cfg_deviceoptions_init();
                             cfg_dev_ptr->type = CFG_DISK; }
           diskoptions
         ENDSECTION        { cfg_diskoptions_check(); }
       | SECTION TTY       { cfg_deviceoptions_init();
                             cfg_dev_ptr->type = CFG_TTY; }
           ttyoptions
         ENDSECTION        { cfg_ttyoptions_check(); }
       | SECTION NIC       { cfg_deviceoptions_init();
                             cfg_dev_ptr->type = CFG_NIC; }
           nicoptions
         ENDSECTION        { cfg_nicoptions_check(); }
       | SECTION PLUGIN    { cfg_deviceoptions_init();
                             cfg_dev_ptr->type = CFG_PLUGIN; }
           plugoptions
         ENDSECTION        { cfg_plugoptions_check(); }
;

simoptions: simoption simoptions
          | simoption
;

simoption: CLOCKSPEED INTEGER32 { cfg_simoption_clockspeed($2); }
          | MEMORY INTEGER32    { cfg_simoption_memory($2); }
          | CPUS INTEGER32      { cfg_simoption_cpus($2); }
          | CPUIRQ INTEGER32    { cfg_simoption_cpuirq($2); }
          | BIGENDIAN           { cfg_simoption_endianness(1); }
          | LITTLEENDIAN        { cfg_simoption_endianness(0); }
;

diskoptions: diskoption diskoptions
           | diskoption
;

diskoption : VENDOR STRING
             { cfg_dev_ptr->vendor = $2; }
           | IRQ INTEGER32
             { cfg_dev_ptr->irq  = $2; }
           | SECTORSIZE INTEGER32
             { cfg_dev_ptr->sectorsize = $2; }
           | CYLINDERS INTEGER32
             { cfg_dev_ptr->numcylinders = $2; }
           | NUMSECTORS INTEGER32
             { cfg_dev_ptr->numsectors = $2; }
           | ROTTIME INTEGER32
             { cfg_dev_ptr->time_rot = $2; }
           | SEEKTIME INTEGER32
             { cfg_dev_ptr->time_fullseek = $2; }
           | FILENAME STRING
             { cfg_dev_ptr->filename = strdup($2); }
;

ttyoptions: ttyoption ttyoptions
          | ttyoption
;

ttyoption: VENDOR STRING
            { cfg_dev_ptr->vendor = $2; }
         | IRQ INTEGER32
            { cfg_dev_ptr->irq = $2; }
         | UNIXSOCKET STRING
            { cfg_dev_ptr->name = cfg_checksocketname($2);
              cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_UNIX); }
         | TCPHOST STRING
            { cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_NET);
	      cfg_dev_ptr->name = $2; }
         | SOCKETLISTEN
            { cfg_dev_ptr->listen = 1; }
         | PORT INTEGER32
            { cfg_dev_ptr->port = $2; }
         | SENDDELAY INTEGER32
            { cfg_dev_ptr->send_delay = $2; }
;

nicoptions: nicoption nicoptions
        | nicoption
;

nicoption: VENDOR STRING
           { cfg_dev_ptr->vendor = $2; }
         | IRQ INTEGER32
           { cfg_dev_ptr->irq = $2; }
         | MTU INTEGER32
           { cfg_dev_ptr->mtu = $2; }
         | MAC INTEGER32
           { cfg_dev_ptr->hwaddr = $2; }
         | RELIABILITY INTEGER32
           { cfg_dev_ptr->reliability = $2; }
         | DMADELAY INTEGER32
           { cfg_dev_ptr->dma_delay = $2; }
         | SENDDELAY INTEGER32
           { cfg_dev_ptr->send_delay = $2; }
         | UNIXSOCKET STRING
            { cfg_dev_ptr->name = cfg_checksocketname($2);
              cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_UNIX); }
         | UDPHOST STRING
            { cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_NET);
              if (strlen($2) > 0)
                cfg_dev_ptr->name = $2;
              else
                 cfg_dev_ptr->name = NULL;
            }
         | PORT INTEGER32
            { cfg_dev_ptr->port = $2; }
;

plugoptions: plugoption plugoptions
          | plugoption
;

plugoption: OPTIONS STRING
            { cfg_dev_ptr->options = $2; }
         | ASYNC
            { cfg_dev_ptr->async = 1; }
         | IRQ INTEGER32
            { cfg_dev_ptr->irq = $2; }
         | UNIXSOCKET STRING
            { cfg_dev_ptr->name = cfg_checksocketname($2);
              cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_UNIX); }
         | TCPHOST STRING
            { cfg_dev_ptr->domain = cfg_socketdomain(CFG_SOCKET_NET);
	      cfg_dev_ptr->name = $2; }
         | SOCKETLISTEN
            { cfg_dev_ptr->listen = 1; }
         | PORT INTEGER32
            { cfg_dev_ptr->port = $2; }
;

%%

