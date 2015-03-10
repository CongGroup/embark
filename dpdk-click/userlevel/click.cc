// -*- c-basic-offset: 4 -*-
/*
 * click.cc -- user-level Click main program
 * Eddie Kohler
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2000 Mazu Networks, Inc.
 * Copyright (c) 2001-2003 International Computer Science Institute
 * Copyright (c) 2004-2006 Regents of the University of California
 * Copyright (c) 2008-2009 Meraki, Inc.
 * Copyright (c) 1999-2011 Eddie Kohler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

/* modified by Maziar, this click+ is without dev-naming capability, uses portid's */
/* sgobriel, remodified enable dev-naming*/ 
#include <iostream>
#include <click/config.h>
#include <click/pathvars.h>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#if HAVE_EXECINFO_H
# include <execinfo.h>
#endif

/* dpdk stuff */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
//#include <sys/types.h>
#include <string.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sys/syscall.h>

#include <rte_config.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_tailq.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_ring.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
//#include <rte_device_set.h>
#include <click/dpdk.h>

#include <click/lexer.hh>
#include <click/routerthread.hh>
#include <click/router.hh>
#include <click/master.hh>
#include <click/error.hh>
#include <click/timer.hh>
#include <click/straccum.hh>
#include <click/clp.h>
#include <click/archive.hh>
#include <click/glue.hh>
#include <click/driver.hh>
#include <click/userutils.hh>
#include <click/args.hh>
#include <click/handlercall.hh>
#include "elements/standard/quitwatcher.hh"
#include "elements/userlevel/controlsocket.hh"
CLICK_USING_DECLS

#define HELP_OPT		300
#define VERSION_OPT		301
#define CLICKPATH_OPT		302
#define ROUTER_OPT		303
#define EXPRESSION_OPT		304
#define QUIT_OPT		305
#define OUTPUT_OPT		306
#define HANDLER_OPT		307
#define TIME_OPT		308
#define PORT_OPT		310
#define UNIX_SOCKET_OPT		311
#define NO_WARNINGS_OPT		312
#define WARNINGS_OPT		313
#define ALLOW_RECONFIG_OPT	314
#define EXIT_HANDLER_OPT	315
#define THREADS_OPT		316
#define SIMTIME_OPT		317
#define SOCKET_OPT		318
/*sgobriel, add defines for new click clp_option*/
#define BINDDEV_OPT		320
#define LIST_BINDDEV_OPT	321
#define TMP_FILE "/tmp/portIDmapping"


static const Clp_Option options[] = {
  /*
   * sgobriel, add bindlist and unbind flag options so click doesn't complain
   * not parsed by click but passed to dpdk in the dpdk arguments
   */ 
  { 0, 'B', BINDDEV_OPT, Clp_ValString, 0},
  { 0, 'I', LIST_BINDDEV_OPT, Clp_ValString, 0},
  /*sgobriel, end changes*/
  { "allow-reconfigure", 'R', ALLOW_RECONFIG_OPT, 0, Clp_Negate },
  { "clickpath", 'C', CLICKPATH_OPT, Clp_ValString, 0 },
  { "expression", 'e', EXPRESSION_OPT, Clp_ValString, 0 },
  { "file", 'f', ROUTER_OPT, Clp_ValString, 0 },
  { "handler", 'h', HANDLER_OPT, Clp_ValString, 0 },
  { "help", 0, HELP_OPT, 0, 0 },
  { "output", 'o', OUTPUT_OPT, Clp_ValString, 0 },
  { "socket", 0, SOCKET_OPT, Clp_ValInt, 0 },
  { "port", 'p', PORT_OPT, Clp_ValString, 0 },
  { "quit", 'q', QUIT_OPT, 0, 0 },
  { "simtime", 0, SIMTIME_OPT, Clp_ValDouble, Clp_Optional },
  { "simulation-time", 0, SIMTIME_OPT, Clp_ValDouble, Clp_Optional },
  { "threads", 'j', THREADS_OPT, Clp_ValInt, 0 },
  { "time", 't', TIME_OPT, 0, 0 },
  { "unix-socket", 'u', UNIX_SOCKET_OPT, Clp_ValString, 0 },
  { "version", 'v', VERSION_OPT, 0, 0 },
  { "warnings", 0, WARNINGS_OPT, 0, Clp_Negate },
  { "exit-handler", 'x', EXIT_HANDLER_OPT, Clp_ValString, 0 },
  { 0, 'w', NO_WARNINGS_OPT, 0, Clp_Negate },
};

static const char *program_name;

struct rte_mempool * pktmbuf_pool = NULL;
//struct rte_mempool * send_pktmbuf_pool = NULL;
//struct rte_eth_conf *port_conf; 

/* ethernet addresses of ports */
static struct ether_addr dpdk_ports_eth_addr[DPDK_MAX_PORTS];

/* mask of enabled ports */
static uint32_t dpdk_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t dpdk_dst_ports[DPDK_MAX_PORTS];

static unsigned int dpdk_rx_queue_per_lcore = 1;

struct mbuf_table {
  unsigned len;
  struct rte_mbuf *m_table[MAX_PKT_BURST];
};

unsigned master_lcore_id;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16

/*sgobriel, add structure to hold bind list and port IDs*/
struct bind_info {
  char device_name[20];
  uint8_t device_id;
};
struct bind_info click_bindlist[DPDK_MAX_PORTS];
static uint8_t n_click_bindlist = 0;
static uint8_t unbind_flag = 0;
static uint8_t bindlist_flag = 0;
/*sgobriel, end changes*/


/* A tsc-based timer responsible for triggering statistics printout */
#define TIMER_MILLISECOND 2000000ULL /* around 1ms at 2 Ghz */
#define MAX_TIMER_PERIOD 86400 /* 1 day max */
static int64_t timer_period = 10 * TIMER_MILLISECOND * 1000; /* default period is 10 seconds */
//end dpdk consts

  void
short_usage()
{
  fprintf(stderr, "Usage: %s [OPTION]... [ROUTERFILE]\n\
      Try '%s --help' for more information.\n",
      program_name, program_name);
}

  void
usage()
{
  printf("\
      'Click' runs a Click router configuration at user level. It installs the\n\
      configuration, reporting any errors to standard error, and then generally runs\n\
      until interrupted.\n\
      \n\
      Usage: %s [OPTION]... [ROUTERFILE]\n\
      \n\
      Options:\n\
      -f, --file FILE               Read router configuration from FILE.\n\
      -e, --expression EXPR         Use EXPR as router configuration.\n\
      -j, --threads N               Start N threads (default 1).\n\
      -p, --port PORT               Listen for control connections on TCP port.\n\
      -u, --unix-socket FILE        Listen for control connections on Unix socket.\n\
      --socket FD               Add a file descriptor control connection.\n\
      -R, --allow-reconfigure       Provide a writable 'hotconfig' handler.\n\
      -h, --handler ELEMENT.H       Call ELEMENT's read handler H after running\n\
      driver and print result to standard output.\n\
      -x, --exit-handler ELEMENT.H  Use handler ELEMENT.H value for exit status.\n\
      -o, --output FILE             Write flat configuration to FILE.\n\
      -q, --quit                    Do not run driver.\n\
      -t, --time                    Print information on how long driver took.\n\
      -w, --no-warnings             Do not print warnings.\n\
      --simtime                 Run in simulation time.\n\
      -C, --clickpath PATH          Use PATH for CLICKPATH.\n\
      --help                    Print this message and exit.\n\
      -v, --version                 Print version number and exit.\n\
      -B,                           FILENAME of bindlist file e.g. -B filename\n\
      contents of file are space delimited.\n\
      -I,                           comma separated list between brackets of ethernet devives to bind to.\n\
      e.g. -I [eth1,eth2,eth3].\n\
      \n\
      Report bugs to <click@pdos.lcs.mit.edu>.\n", program_name);
}

static Router *router;
static ErrorHandler *errh;
static bool started = 0;

extern "C" {
  static void
    stop_signal_handler(int sig)
    {
#if !HAVE_SIGACTION
      signal(sig, SIG_DFL);
#endif
      if (!started)
        kill(getpid(), sig);
      else
        router->set_runcount(Router::STOP_RUNCOUNT);
    }

#if HAVE_EXECINFO_H
  static void
    catch_dump_signal(int sig)
    {
      (void) sig;

      /* reset these signals so if we do something bad we just exit */
      click_signal(SIGSEGV, SIG_DFL, false);
      click_signal(SIGBUS, SIG_DFL, false);
      click_signal(SIGILL, SIG_DFL, false);
      click_signal(SIGABRT, SIG_DFL, false);
      click_signal(SIGFPE, SIG_DFL, false);

      /* dump the results to standard error */
      void *return_addrs[50];
      int naddrs = backtrace(return_addrs, sizeof(return_addrs) / sizeof(void *));
      backtrace_symbols_fd(return_addrs, naddrs, STDERR_FILENO);

      /* dump core and quit */
      abort();
    }
#endif
}


// report handler results

  static int
call_read_handler(Element *e, String handler_name,
    bool print_name, ErrorHandler *errh)
{
  const Handler *rh = Router::handler(e, handler_name);
  String full_name = Handler::unparse_name(e, handler_name);
  if (!rh || !rh->visible())
    return errh->error("no %<%s%> handler", full_name.c_str());
  else if (!rh->read_visible())
    return errh->error("%<%s%> is a write handler", full_name.c_str());

  if (print_name)
    fprintf(stdout, "%s:\n", full_name.c_str());
  String result = rh->call_read(e);
  if (!rh->raw() && result && result.back() != '\n')
    result += '\n';
  fputs(result.c_str(), stdout);
  if (print_name)
    fputs("\n", stdout);

  return 0;
}

  static int
expand_handler_elements(const String& pattern, const String& handler_name,
    Vector<Element*>& elements, Router* router)
{
  // first try element name
  if (Element* e = router->find(pattern)) {
    elements.push_back(e);
    return 1;
  }
  // check if we have a pattern
  bool is_pattern = false;
  for (const char* s = pattern.begin(); s < pattern.end(); s++)
    if (*s == '?' || *s == '*' || *s == '[') {
      is_pattern = true;
      break;
    }
  // check pattern or type
  bool any = false;
  for (int i = 0; i < router->nelements(); i++)
    if (is_pattern
        ? glob_match(router->ename(i), pattern)
        : router->element(i)->cast(pattern.c_str()) != 0) {
      any = true;
      const Handler* h = Router::handler(router->element(i), handler_name);
      if (h && h->read_visible())
        elements.push_back(router->element(i));
    }
  if (!any)
    return errh->error((is_pattern ? "no element matching %<%s%>" : "no element %<%s%>"), pattern.c_str());
  else
    return 2;
}

  static int
call_read_handlers(Vector<String> &handlers, ErrorHandler *errh)
{
  Vector<Element *> handler_elements;
  Vector<String> handler_names;
  bool print_names = (handlers.size() > 1);
  int before = errh->nerrors();

  // expand handler names
  for (int i = 0; i < handlers.size(); i++) {
    const char *dot = find(handlers[i], '.');
    if (dot == handlers[i].end()) {
      call_read_handler(router->root_element(), handlers[i], print_names, errh);
      continue;
    }

    String element_name = handlers[i].substring(handlers[i].begin(), dot);
    String handler_name = handlers[i].substring(dot + 1, handlers[i].end());

    Vector<Element*> elements;
    int retval = expand_handler_elements(element_name, handler_name, elements, router);
    if (retval >= 0)
      for (int j = 0; j < elements.size(); j++)
        call_read_handler(elements[j], handler_name, print_names || retval > 1, errh);
  }

  return (errh->nerrors() == before ? 0 : -1);
}


// hotswapping

static Router *hotswap_router;
static Router *hotswap_thunk_router;
static bool hotswap_hook(Task *, void *);
static Task hotswap_task(hotswap_hook, 0);

  static bool
hotswap_hook(Task *, void *)
{
  hotswap_thunk_router->set_foreground(false);
  hotswap_router->activate(ErrorHandler::default_handler());
  router->unuse();
  router = hotswap_router;
  router->use();
  hotswap_router = 0;
  return true;
}

// switching configurations

static Vector<String> cs_unix_sockets;
static Vector<String> cs_ports;
static Vector<String> cs_sockets;
static bool warnings = true;
static int nthreads = 1;

  static String
click_driver_control_socket_name(int number)
{
  if (!number)
    return "click_driver@@ControlSocket";
  else
    return "click_driver@@ControlSocket@" + String(number);
}

  static Router *
parse_configuration(const String &text, bool text_is_expr, bool hotswap,
    ErrorHandler *errh)
{
  Master *new_master = 0, *master;
  if (router)
    master = router->master();
  else
    master = new_master = new Master(nthreads);

  Router *r = click_read_router(text, text_is_expr, errh, false, master);
  if (!r) {
    delete new_master;
    return 0;
  }

  // add new ControlSockets
  String retries = (hotswap ? ", RETRIES 1, RETRY_WARNINGS false" : "");
  int ncs = 0;
  for (String *it = cs_ports.begin(); it != cs_ports.end(); ++it, ++ncs)
    r->add_element(new ControlSocket, click_driver_control_socket_name(ncs), "TCP, " + *it + retries, "click", 0);
  for (String *it = cs_unix_sockets.begin(); it != cs_unix_sockets.end(); ++it, ++ncs)
    r->add_element(new ControlSocket, click_driver_control_socket_name(ncs), "UNIX, " + *it + retries, "click", 0);
  for (String *it = cs_sockets.begin(); it != cs_sockets.end(); ++it, ++ncs)
    r->add_element(new ControlSocket, click_driver_control_socket_name(ncs), "SOCKET, " + *it + retries, "click", 0);

  // catch signals (only need to do the first time)
  if (!hotswap) {
    // catch control-C and SIGTERM
    click_signal(SIGINT, stop_signal_handler, true);
    click_signal(SIGTERM, stop_signal_handler, true);
    // ignore SIGPIPE
    click_signal(SIGPIPE, SIG_IGN, false);

#if HAVE_EXECINFO_H
    const char *click_backtrace = getenv("CLICK_BACKTRACE");
    bool do_click_backtrace;
    if (click_backtrace && (!BoolArg().parse(click_backtrace, do_click_backtrace)
          || do_click_backtrace)) {
      click_signal(SIGSEGV, catch_dump_signal, false);
      click_signal(SIGBUS, catch_dump_signal, false);
      click_signal(SIGILL, catch_dump_signal, false);
      click_signal(SIGABRT, catch_dump_signal, false);
      click_signal(SIGFPE, catch_dump_signal, false);
    }
#endif
  }

  // register hotswap router on new router
  if (hotswap && router && router->initialized())
    r->set_hotswap_router(router);

  if (errh->nerrors() > 0 || r->initialize(errh) < 0) {
    delete r;
    delete new_master;
    return 0;
  } else
    return r;
}

  static int
hotconfig_handler(const String &text, Element *, void *, ErrorHandler *errh)
{
  if (Router *q = parse_configuration(text, true, true, errh)) {
    if (hotswap_router)
      hotswap_router->unuse();
    hotswap_router = q;
    hotswap_thunk_router->set_foreground(true);
    hotswap_task.reschedule();
    return 0;
  } else
    return -EINVAL;
}


// timewarping

  static String
timewarp_read_handler(Element *, void *)
{
  if (Timestamp::warp_class() == Timestamp::warp_simulation)
    return "simulation";
  else if (Timestamp::warp_class() == Timestamp::warp_nowait)
    return "nowait";
  else
    return String(Timestamp::warp_speed());
}

  static int
timewarp_write_handler(const String &text, Element *, void *, ErrorHandler *errh)
{
  if (text == "nowait")
    Timestamp::warp_set_class(Timestamp::warp_nowait);
  else {
    double factor;
    if (!DoubleArg().parse(text, factor))
      return errh->error("expected double");
    else if (factor <= 0)
      return errh->error("timefactor must be > 0");
    Timestamp::warp_set_class(Timestamp::warp_linear, factor);
  }
  return 0;
}

//dpdk functions

/* sgobriel, a function returns portID given device name, return -1 if not found*/
  int
get_portID(String devname)
{	int i;	  
  for (i = 0; i < n_click_bindlist; i++){
    String bindlist_devname(click_bindlist[i].device_name);	
    if(devname.compare(bindlist_devname)==0)					
      return click_bindlist[i].device_id;
  }
  return -1;


}

/* sgobriel, a function to log portID and device name to a tmp file */
  void
dump_portID(void)
{
  FILE *f=NULL;
  char data_buf[1024];
  int i, n;

  f= fopen(TMP_FILE, "w");
  if (f == NULL){
    printf("%s(): Failed to create tmp file %s\n", __func__, TMP_FILE);
    exit(0);
  }
  printf("Logging Device Names and PortIDs to %s\n", TMP_FILE);
  n = snprintf(data_buf, sizeof(data_buf),"#ethernet_device_name dpdk_port\n");
  if( (n<0) || (n>=(int) sizeof(data_buf)) ){
    printf("%s(): rte_snprintf failed\n", __func__);
    exit(0);
  }
  if(fwrite(data_buf, 1, n, f)==0){
    printf("%s(): could not write to %s", __func__, TMP_FILE);
    exit(0);
  }
  for (i = 0; i < n_click_bindlist; i++){
    n = snprintf(data_buf, sizeof(data_buf), "%s %d\n", click_bindlist[i].device_name, click_bindlist[i].device_id);
    fwrite(data_buf, 1, n, f);
  }

  fclose(f);

}

/* sgobriel, end changes*/


/* display usage */
  static void
dpdk_usage(const char *prgname)
{
  printf("%s [EAL options] -- -p PORTMASK [-q NQ]\n"
      "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
      "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
      "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n",
      prgname);
}

  static int
dpdk_parse_portmask(const char *portmask)
{
  char *end = NULL;
  unsigned long pm;

  /* parse hexadecimal string */
  pm = strtoul(portmask, &end, 16);
  if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
    return -1;

  if (pm == 0)
    return -1;

  return pm;
}

  static unsigned int
dpdk_parse_nqueue(const char *q_arg)
{
  char *end = NULL;
  unsigned long n;

  /* parse hexadecimal string */
  n = strtoul(q_arg, &end, 10);
  if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
    return 0;
  if (n == 0)
    return 0;
  if (n >= MAX_RX_QUEUE_PER_LCORE)
    return 0;

  return n;
}

  static int
dpdk_parse_timer_period(const char *q_arg)
{
  char *end = NULL;
  int n;

  /* parse number string */
  n = strtol(q_arg, &end, 10);
  if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
    return -1;
  if (n >= MAX_TIMER_PERIOD)
    return -1;

  return n;
}

/* Parse the argument given in the command line of the application */
  static int
dpdk_parse_args(int argc, char **argv)
{
  int opt, ret;
  char **argvopt;
  int option_index;
  char *prgname = argv[0];
  static struct option lgopts[] = {
    {NULL, 0, 0, 0}
  };

  argvopt = argv;

  while ((opt = getopt_long(argc, argvopt, "p:B:I:q:T:",
          lgopts, &option_index)) != EOF) {

    switch (opt) {
      /* portmask */
      case 'p':
        dpdk_enabled_port_mask = dpdk_parse_portmask(optarg);
        if (dpdk_enabled_port_mask == 0) {
          printf("invalid portmask\n");
          dpdk_usage(prgname);
          return -1;
        }
        break;
      case 'B':
        break;

      case 'I':
        break;


        /* nqueue */
      case 'q':
        dpdk_rx_queue_per_lcore = dpdk_parse_nqueue(optarg);
        if (dpdk_rx_queue_per_lcore == 0) {
          printf("invalid queue number\n");
          dpdk_usage(prgname);
          return -1;
        }
        break;

        /* timer period */
      case 'T':
        timer_period = dpdk_parse_timer_period(optarg) * 1000 * TIMER_MILLISECOND;
        if (timer_period < 0) {
          printf("invalid timer period\n");
          dpdk_usage(prgname);
          return -1;
        }
        break;

        /* long options */
      case 0:
        dpdk_usage(prgname);
        return -1;

      default:
        dpdk_usage(prgname);
        return -1;
    }
  }

  if (optind >= 0)
    argv[optind-1] = prgname;

  ret = optind-1;
  optind = 0; /* reset getopt lib */
  return ret;
}
//end dpdk fns

// main

  static void
round_timeval(struct timeval *tv, int usec_divider)
{
  tv->tv_usec = (tv->tv_usec + usec_divider / 2) / usec_divider;
  if (tv->tv_usec >= 1000000 / usec_divider) {
    tv->tv_usec = 0;
    ++tv->tv_sec;
  }
}

#if HAVE_MULTITHREAD
Vector<pthread_t> other_threads;
#endif

#if HAVE_MULTITHREAD
extern "C" {
  static void *thread_driver(void *user_data)
  {
    RouterThread *thread = static_cast<RouterThread *>(user_data);
    pthread_t thread_id;
    unsigned lcore_id;

    thread_id = pthread_self();
    for (lcore_id = 0; lcore_id < other_threads.size(); lcore_id++)
      if (thread_id == other_threads[lcore_id])
        break;

    if (lcore_id == other_threads.size())
      rte_panic(" Click Thread is not recognized for lcore operation\n");

    if (lcore_id + master_lcore_id == RTE_MAX_LCORE)
      rte_panic("cannot retrieve lcore id for Click Thread\n");

    //RTE_LOG(DEBUG, EAL, "lCore %u is ready for Click Thread(tid=%x) (gtid=%ld)\n",    
    //      lcore_id+1, (int)thread_id, syscall(SYS_gettid));

    click_eal_set_lcore_id(lcore_id + 1 + master_lcore_id);

    lcore_id = rte_lcore_id();
    printf("lcore %u is ready for Click Thread(tid=%x) (gtid=%ld)\n",
        lcore_id, (int)thread_id, syscall(SYS_gettid));

    std::cout << "ASSIGNING!! " << pthread_self() << " to " << thread->thread_id() << std::endl;
    thread->driver();
    return 0;
  }
}
#endif

  static int
cleanup(Clp_Parser *clp, int exit_value)
{
  Clp_DeleteParser(clp);
  click_static_cleanup();
  return exit_value;
}


/* sgobriel, to parse bindlist file given with -B option 
 * and fill in the click_bindlist strcuture
 * or a list in braces with -I option
 */
  void
parse_bindlist(int argc, char **argv)
{
  int i, list=0, order=0;
  FILE * pFile;
  char filename[150];
  String tmp_string, tmp_name;  

  for(i=0; i<argc; i++){	    
    if (String(argv[i]) == "-B"){
      if(bindlist_flag){
        printf("Invalid duplicate Bind List Options Specified\n Use either -B [filename] or -I [{dev1, dev2, ..}] argument\n");
        exit(1);
      }
      bindlist_flag=1;
      list = 0;
      snprintf(filename, sizeof(filename), "%s", argv[i+1]);
      printf("BindList file is %s\n", filename);
    }
    if (String(argv[i]) == "-I"){
      if(bindlist_flag){
        printf("Invalid duplicate Bind List Options Specified\n Use either -B [filename] or -I [{dev1, dev2, ..}] argument\n");
        exit(1);
      }
      bindlist_flag=1;
      list = i+1;
    }
  }

  if(bindlist_flag){
    if(!list){ //filename -B option specified
      pFile = fopen (filename,"r");
      if (pFile == NULL){
        printf("%s(): couldn't open bindlist file\n", __func__);
        exit(1);
      }
      while (fscanf(pFile, "%s", click_bindlist[order].device_name)!=EOF){
        //printf("device name =%s\n",click_bindlist[order].device_name);
        click_bindlist[order].device_id=order;
        order++;	
      }
      n_click_bindlist=order;
      fclose(pFile);
    }

    else{ //device list -I option specified
      if(argv[list][0]!='[' || argv[list][strlen(argv[list])-1]!=']'){
        printf("Error in device list syntax\n");
        printf("Device list should be specified between brackets as -I [dev1,dev2,..]\n");
        printf("NOTE: device list is comma separated with no spaces\n");
        exit(1);
      }
      //strip preceding '[' and ending ']'
      tmp_string = String(argv[list]);
      tmp_string = tmp_string.substring(1); //strip preceding character
      tmp_string = tmp_string.substring(0,tmp_string.length()-1); //strip last character
      while(tmp_string.length() > 1){
        const char *comma = find(tmp_string, ',');
        tmp_name = tmp_string.substring(tmp_string.begin(), comma);
        tmp_string = tmp_string.substring(comma + 1, tmp_string.end());
        snprintf(click_bindlist[order].device_name, sizeof(click_bindlist[0].device_name), "%s", tmp_name.c_str());
        click_bindlist[order].device_id=order;
        order++;
      }
      n_click_bindlist=order;

    }	    
  }
}


/* sgobriel, debug only function to fill bindlist manually */
void 
fill_bindlist_manual(void){
  bindlist_flag=1;
  snprintf(click_bindlist[0].device_name, sizeof(click_bindlist[0].device_name), "%s", "eth10");
  click_bindlist[0].device_id=0;

  snprintf(click_bindlist[1].device_name, sizeof(click_bindlist[1].device_name), "%s", "eth12");
  click_bindlist[1].device_id=1;

  snprintf(click_bindlist[2].device_name, sizeof(click_bindlist[2].device_name), "%s", "eth13");
  click_bindlist[2].device_id=2;

  n_click_bindlist=3;
}

/*sgobriel, end changes*/

  int
main(int argc, char **argv)
{

  /*sgobriel, add bind list and unbind flag as a command line argument*/
  /* use manual parsing since getopt and CLP contradicts each other */
  int i=0;

  //fill_bindlist_manual(); // debug function
  parse_bindlist(argc, argv);

  printf("Click Bind List:");
  for(i=0; i< n_click_bindlist;i++)
    printf( " %s", click_bindlist[i].device_name);
  printf("\n");
  /*sgobriel, end changes*/

  click_static_initialize();
  errh = ErrorHandler::default_handler();

  // read command line arguments
  Clp_Parser *clp =
    Clp_NewParser(argc, argv, sizeof(options) / sizeof(options[0]), options);
  program_name = Clp_ProgramName(clp);

  const char *router_file = 0;
  bool file_is_expr = false;
  const char *output_file = 0;
  bool quit_immediately = false;
  bool report_time = false;
  bool allow_reconfigure = false;
  Vector<String> handlers;
  String exit_handler;

  /*dpdk init*/
  int dpdk_argc=1+4;
  char *dpdk_argv[] = {"forwd", "-c", "1", "-n", "4"};
  //debug
  //char *dpdk_argv[] = {"click+", "-c", "1", "-n", "4", "-B", "eth9", "eth12", "eth13", "--", "-p", "F"}; 
  // 
  /*sgobriel, add bindlist if enabled*/
  if(bindlist_flag){
    dpdk_argv[dpdk_argc]="-B";
    for(i=0; i< n_click_bindlist;i++){
      dpdk_argc++;
      dpdk_argv[dpdk_argc]=click_bindlist[i].device_name;
    }
    dpdk_argc++;

    /*add portmask*/
    char buffer[50];	
    dpdk_argv[dpdk_argc]="--";
    dpdk_argc++;
    dpdk_argv[dpdk_argc]="-p";
    dpdk_argc++;
    unsigned long mask=((uint32_t)1 << n_click_bindlist) - 1;	
    sprintf(buffer, "%02lx", mask);
    printf("port mask=%s\n", buffer);
    dpdk_argv[dpdk_argc]=buffer;
    dpdk_argc++;
  }  
  /* sgobriel, endchanges*/ 

  struct lcore_queue_conf *qconf;
  struct rte_eth_dev_info dev_info;
  struct rte_eth_link link;
  int ret;
  unsigned int nb_ports, nb_lcores;
  unsigned portid, last_port, queueid = 0;
  unsigned lcore_id, rx_lcore_id;
  unsigned n_tx_queue, max_tx_queues;
  unsigned nb_ports_in_mask = 0;

  struct rte_eth_conf port_conf;
  struct rte_eth_conf *ptr_port_conf;
  //set up port_conf, better do it this way since no . notation exists here      
  //port_conf = (struct rte_eth_conf *) malloc(sizeof(struct rte_eth_conf));
  memset(&port_conf, 0, sizeof(port_conf));
  ptr_port_conf = &port_conf;

  //printf("???? size of port_conf %d\n", sizeof(port_conf));
  port_conf.rxmode.split_hdr_size = SPLIT_HDR_SIZE;
  port_conf.rxmode.header_split   = HEADER_SPLIT;
  port_conf.rxmode.hw_ip_checksum = HW_IP_CHECKSUM;
  port_conf.rxmode.hw_vlan_filter = HW_VLAN_FILTER;
  port_conf.rxmode.jumbo_frame    = JUMBO_FRAME;
  port_conf.rxmode.hw_strip_crc   = HW_STRIP_CRC; /**< CRC stripped by hardware */

  //RRS feature setup on the card
#if CLICK_RSS_ON
  port_conf.rxmode.mq_mode = ETH_RSS;
  ptr_port_conf->rx_adv_conf.rss_conf.rss_key = NULL;
  ptr_port_conf->rx_adv_conf.rss_conf.rss_hf = ETH_RSS_IPV4;
#endif

  printf("rss_hf=%u\n", port_conf.rx_adv_conf.rss_conf.rss_hf);

  /* init EAL */
  ret = rte_eal_init(dpdk_argc, dpdk_argv);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
  dpdk_argc -= ret;
  //dpdk_argv = dpdk_argv + ret;

  /* parse application arguments (after the EAL ones) */
  ret = dpdk_parse_args(dpdk_argc, dpdk_argv + ret);
  if (ret < 0)
    rte_exit(EXIT_FAILURE, "Invalid DPDK arguments\n");

  /* create the mbuf pool */
  pktmbuf_pool =
    rte_mempool_create("mbuf_pool", NB_MBUF,
        MBUF_SIZE, 64,
        sizeof(struct rte_pktmbuf_pool_private),
        rte_pktmbuf_pool_init, NULL,
        rte_pktmbuf_init, NULL,
        rte_socket_id(), 0);

  if (pktmbuf_pool == NULL)
    rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");
  /*
     send_pktmbuf_pool =
     rte_mempool_create("mbuf_pool_send", NB_MBUF -1,
     MBUF_SIZE, 32,
     sizeof(struct rte_pktmbuf_pool_private),
     rte_pktmbuf_pool_init, NULL,
     rte_pktmbuf_init, NULL,
     SOCKET0, 0);
     if (send_pktmbuf_pool == NULL)
     rte_exit(EXIT_FAILURE, "Cannot init mbuf send pool\n");
     */

  /* init driver(s) */
#ifdef RTE_LIBRTE_IGB_PMD
  if (rte_igb_pmd_init() < 0)
    rte_exit(EXIT_FAILURE, "Cannot init igb pmd\n");
#endif
#ifdef RTE_LIBRTE_IXGBE_PMD
  if (rte_ixgbe_pmd_init() < 0)
    rte_exit(EXIT_FAILURE, "Cannot init ixgbe pmd\n");
#endif

  if (rte_eal_pci_probe() < 0)
    rte_exit(EXIT_FAILURE, "Cannot probe PCI\n");

  nb_ports = rte_eth_dev_count();
  if (nb_ports == 0)
    rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

  if (nb_ports > DPDK_MAX_PORTS)
    nb_ports = DPDK_MAX_PORTS;

  /*sgobriel, check that all ports in bind list has been initialized*/
  if(bindlist_flag){
    if (nb_ports != n_click_bindlist)
      rte_exit(EXIT_FAILURE, "Failed to initialized ports in bindlist - bye\n");
    //log portIDs to tmp file
    dump_portID();
  }
  /*sgobriel, end changes*/


  nb_lcores = rte_lcore_count();

  //printf("nb_lcores = %u, nb_ports = %u\n", nb_lcores, nb_ports);

  /* reset dpdk_dst_ports */
  for (portid = 0; portid < DPDK_MAX_PORTS; portid++)
    dpdk_dst_ports[portid] = 0;
  last_port = 0;

  /*
   * Each logical core is assigned a dedicated TX queue on each port.
   * Compute the maximum number of TX queues that can be used.
   */
  max_tx_queues = nb_lcores;
  for (portid = 0; portid < nb_ports; portid++) {
    /* skip ports that are not enabled */
    if ((dpdk_enabled_port_mask & (1 << portid)) == 0)
      continue;

    if (nb_ports_in_mask % 2) {
      dpdk_dst_ports[portid] = last_port;
      dpdk_dst_ports[last_port] = portid;
    }
    else
      last_port = portid;

    nb_ports_in_mask++;

    rte_eth_dev_info_get((uint8_t) portid, &dev_info);
    if (max_tx_queues > dev_info.max_tx_queues)
      max_tx_queues = dev_info.max_tx_queues;
  }
  if (nb_ports_in_mask < 2 || nb_ports_in_mask % 2) {
    //rte_exit(EXIT_FAILURE, "invalid number of ports in portmask. "
    //       "Should be an even number.\n");
  }

  /* Initialise each port */
  for (portid = 0; portid < nb_ports; portid++) {

    /* skip ports that are not enabled */
    if ((dpdk_enabled_port_mask & (1 << portid)) == 0) {
      printf("Skipping disabled port %u\n", portid);
      continue;
    }
    /* init port */
    printf("Initializing port %u... ", portid);
    fflush(stdout);
    printf("rss_hf=%u, port_conf=%p\n", port_conf.rx_adv_conf.rss_conf.rss_hf, &port_conf);
    ret = rte_eth_dev_configure((uint8_t) portid, NUM_RX_QUEUE,
        (uint16_t) NUM_TX_QUEUE, ptr_port_conf);
    printf("rss_hf=%u, port_conf=%p\n", port_conf.rx_adv_conf.rss_conf.rss_hf, &port_conf);
    if (ret < 0)
      rte_exit(EXIT_FAILURE, "Cannot configure device: "
          "err=%d, port=%u\n",
          ret, portid);

    rte_eth_macaddr_get((uint8_t) portid,
        &dpdk_ports_eth_addr[portid]);

    /* init RSS RX queue */
    fflush(stdout);
    for (queueid = 0; queueid < NUM_RX_QUEUE; queueid++) {
      ret = rte_eth_rx_queue_setup((uint8_t) portid, (uint16_t) queueid, nb_rxd,
          rte_socket_id(), &rx_conf,
          pktmbuf_pool);
      if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: "
            "err=%d, port=%u, can't init rx Q %u\n",
            ret, portid, queueid);
    }
    /* init number of tx q's for each port, init only one tx queue for each port for now*/
    for (queueid = 0; queueid < NUM_TX_QUEUE; queueid++) {
      fflush(stdout);
      ret = rte_eth_tx_queue_setup((uint8_t) portid,
          (uint16_t) queueid, nb_txd,
          rte_socket_id(), &tx_conf);
      if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup: "
            "err=%d, port=%u queue=%u\n",
            ret, portid, queueid);
    }

    /* Start device */
    ret = rte_eth_dev_start((uint8_t) portid);
    if (ret < 0)
      rte_exit(EXIT_FAILURE, "rte_eth_dev_start: "
          "err=%d, port=%u\n",
          ret, portid);

    printf("done: ");

    /* get link status */
    rte_eth_link_get((uint8_t) portid, &link);
    if (link.link_status) {
      printf(" Link Up - speed %u Mbps - %s\n",
          (unsigned) link.link_speed,
          (link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
          ("full-duplex") : ("half-duplex\n"));
    } else {
      printf(" Link Down\n");
    }

    rte_eth_promiscuous_enable((uint8_t)portid);

    printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
        portid,
        dpdk_ports_eth_addr[portid].addr_bytes[0],
        dpdk_ports_eth_addr[portid].addr_bytes[1],
        dpdk_ports_eth_addr[portid].addr_bytes[2],
        dpdk_ports_eth_addr[portid].addr_bytes[3],
        dpdk_ports_eth_addr[portid].addr_bytes[4],
        dpdk_ports_eth_addr[portid].addr_bytes[5]);


  }
  //end dpdk

  //click parsing
  while (1) {
    int opt = Clp_Next(clp);
    switch (opt) {

      case ROUTER_OPT:
      case EXPRESSION_OPT:
router_file:
        if (router_file) {
          errh->error("router configuration specified twice");
          goto bad_option;
        }
        router_file = clp->vstr;
        file_is_expr = (opt == EXPRESSION_OPT);
        break;

      case Clp_NotOption:
        for (const char *s = clp->vstr; *s; s++)
          if (*s == '=' && s > clp->vstr) {
            if (!click_lexer()->global_scope().define(String(clp->vstr, s), s + 1, false))
              errh->error("parameter %<%.*s%> multiply defined", s - clp->vstr, clp->vstr);
            goto next_argument;
          } else if (!isalnum((unsigned char) *s) && *s != '_')
            break;
        goto router_file;

      case OUTPUT_OPT:
        if (output_file) {
          errh->error("output file specified twice");
          goto bad_option;
        }
        output_file = clp->vstr;
        break;

      case HANDLER_OPT:
        handlers.push_back(clp->vstr);
        break;

      case EXIT_HANDLER_OPT:
        if (exit_handler) {
          errh->error("--exit-handler specified twice");
          goto bad_option;
        }
        exit_handler = clp->vstr;
        break;

      case PORT_OPT: {
                       uint16_t portno;
                       int portno_int = -1;
                       String vstr(clp->vstr);
                       if (IPPortArg(IP_PROTO_TCP).parse(vstr, portno))
                         cs_ports.push_back(String(portno));
                       else if (vstr && vstr.back() == '+'
                           && IntArg().parse(vstr.substring(0, -1), portno_int)
                           && portno_int > 0 && portno_int < 65536)
                         cs_ports.push_back(String(portno_int) + "+");
                       else {
                         Clp_OptionError(clp, "%<%O%> expects a TCP port number, not %<%s%>", clp->vstr);
                         goto bad_option;
                       }
                       break;
                     }

      case UNIX_SOCKET_OPT:
                     cs_unix_sockets.push_back(clp->vstr);
                     break;

      case SOCKET_OPT:
                     cs_sockets.push_back(clp->vstr);
                     break;

      case ALLOW_RECONFIG_OPT:
                     allow_reconfigure = !clp->negated;
                     break;

      case QUIT_OPT:
                     quit_immediately = true;
                     break;

      case TIME_OPT:
                     report_time = true;
                     break;

      case WARNINGS_OPT:
                     warnings = !clp->negated;
                     break;

      case NO_WARNINGS_OPT:
                     warnings = clp->negated;
                     break;

                     /*sgobriel, add dpdpk options*/	
      case BINDDEV_OPT:
                     //printf("DEBUG0: Bind option set with argument %s \n\n", clp->vstr);
                     break;

      case LIST_BINDDEV_OPT:
                     //printf("DEBUG0: LIST Bind option set with argument %s \n\n", clp->vstr);
                     break;
                     /*sgobriel, end changes*/

      case THREADS_OPT:
                     nthreads = clp->val.i;
                     if (nthreads <= 1)
                       nthreads = 1;
#if !HAVE_MULTITHREAD
                     if (nthreads > 1) {
                       errh->warning("Click was built without multithread support, running single threaded");
                       nthreads = 1;
                     }
#endif
                     break;

      case SIMTIME_OPT: {
                          Timestamp::warp_set_class(Timestamp::warp_simulation);
                          Timestamp simbegin(clp->have_val ? clp->val.d : 1000000000);
                          Timestamp::warp_set_now(simbegin, simbegin);
                          break;
                        }

      case CLICKPATH_OPT:
                        set_clickpath(clp->vstr);
                        break;

      case HELP_OPT:
                        usage();
                        return cleanup(clp, 0);

      case VERSION_OPT:
                        printf("click (Click) %s\n", CLICK_VERSION);
                        printf("Copyright (C) 1999-2001 Massachusetts Institute of Technology\n\
                            Copyright (C) 2001-2003 International Computer Science Institute\n\
                            Copyright (C) 2008-2009 Meraki, Inc.\n\
                            Copyright (C) 2004-2011 Regents of the University of California\n\
                            This is free software; see the source for copying conditions.\n\
                            There is NO warranty, not even for merchantability or fitness for a\n\
                            particular purpose.\n");
                        return cleanup(clp, 0);

bad_option:
      case Clp_BadOption:
                        short_usage();
                        return cleanup(clp, 1);

      case Clp_Done:
                        goto done;

    }
next_argument: ;
  }

done:
  // provide hotconfig handler if asked
  if (allow_reconfigure)
    Router::add_write_handler(0, "hotconfig", hotconfig_handler, 0, Handler::RAW | Handler::NONEXCLUSIVE);
  Router::add_read_handler(0, "timewarp", timewarp_read_handler, 0);
  if (Timestamp::warp_class() != Timestamp::warp_simulation)
    Router::add_write_handler(0, "timewarp", timewarp_write_handler, 0);

  // parse configuration
  router = parse_configuration(router_file, file_is_expr, false, errh);
  if (!router)
    return cleanup(clp, 1);
  router->use();

  int exit_value = 0;

  // output flat configuration
  if (output_file) {
    FILE *f = 0;
    if (strcmp(output_file, "-") != 0) {
      f = fopen(output_file, "w");
      if (!f) {
        errh->error("%s: %s", output_file, strerror(errno));
        exit_value = 1;
      }
    } else
      f = stdout;
    if (f) {
      Element *root = router->root_element();
      String s = Router::handler(root, "flatconfig")->call_read(root);
      ignore_result(fwrite(s.data(), 1, s.length(), f));
      if (f != stdout)
        fclose(f);
    }
  }

  struct rusage before, after;
  getrusage(RUSAGE_SELF, &before);
  Timestamp before_time = Timestamp::now_unwarped();
  Timestamp after_time = Timestamp::uninitialized_t();

  // run driver
  // 10.Apr.2004 - Don't run the router if it has no elements.
  if (!quit_immediately && router->nelements()) {
    started = true;
    router->activate(errh);
    if (allow_reconfigure) {
      hotswap_thunk_router = new Router("", router->master());
      hotswap_thunk_router->initialize(errh);
      hotswap_task.initialize(hotswap_thunk_router->root_element(), false);
      hotswap_thunk_router->activate(false, errh);
    }
#if HAVE_MULTITHREAD
    master_lcore_id = rte_lcore_id();
    for (int t = 1; t < nthreads; ++t) {
      pthread_t p;
      pthread_create(&p, 0, thread_driver, router->master()->thread(t));
      other_threads.push_back(p);
    }
#endif
    router->master()->thread(0)->driver();
  } else if (!quit_immediately && warnings)
    errh->warning("%s: configuration has no elements, exiting", filename_landmark(router_file, file_is_expr));

  after_time.assign_now_unwarped();
  getrusage(RUSAGE_SELF, &after);
  // report time
  if (report_time) {
    struct timeval diff;
    timersub(&after.ru_utime, &before.ru_utime, &diff);
    round_timeval(&diff, 1000);
    printf("%ld.%03ldu", (long)diff.tv_sec, (long)diff.tv_usec);
    timersub(&after.ru_stime, &before.ru_stime, &diff);
    round_timeval(&diff, 1000);
    printf(" %ld.%03lds", (long)diff.tv_sec, (long)diff.tv_usec);
    diff = (after_time - before_time).timeval();
    round_timeval(&diff, 10000);
    printf(" %ld:%02ld.%02ld", (long)(diff.tv_sec/60), (long)(diff.tv_sec%60), (long)diff.tv_usec);
    printf("\n");
  }

  // call handlers
  if (handlers.size())
    if (call_read_handlers(handlers, errh) < 0)
      exit_value = 1;

  // call exit handler
  if (exit_handler) {
    int before = errh->nerrors();
    String exit_string = HandlerCall::call_read(exit_handler, router->root_element(), errh);
    bool b;
    if (errh->nerrors() != before)
      exit_value = -1;
    else if (IntArg().parse(cp_uncomment(exit_string), exit_value))
      /* nada */;
    else if (BoolArg().parse(cp_uncomment(exit_string), b))
      exit_value = (b ? 0 : 1);
    else {
      errh->error("exit handler value should be integer");
      exit_value = -1;
    }
  }

  Master *master = router->master();
  router->unuse();
#if HAVE_MULTITHREAD
  for (int i = 0; i < other_threads.size(); ++i)
    master->thread(i + 1)->wake();
  for (int i = 0; i < other_threads.size(); ++i)
    (void) pthread_join(other_threads[i], 0);
#endif
  delete master;

  return cleanup(clp, exit_value);
}
