/*
 * Copyright (C) 2001  Florian Sander
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#define MODULE_NAME "webseen"
#define MODULE_VERSION "0.3.1"
#define MAKING_WEBSEEN
#include "../module.h"
#include "gseen.mod/gseen.h"
#include <stdlib.h>

#include "webseen.h"

#ifndef EGG_IS_MIN_VER
#define EGG_IS_MIN_VER(ver)             ((ver) <= 10400)
#endif

#if !EGG_IS_MIN_VER(10500)
#define OLDBOT 1
#endif

#ifndef Context
#define Context context
#endif

#ifndef findchan_by_dname
#define findchan_by_dname findchan
#endif

#ifndef my_htonl
#define my_htonl htonl
#endif


#undef global
/* Pointer to the eggdrop core function table. Gets initialized in
 * woobie_start().
 */
static Function *global = NULL, *gseen_funcs = NULL;

char *glob_query;
int glob_fuzzy, glob_seenerror;
double glob_searchtime;

static int offset = 0;

/* global vars for exchanging data between functions */
gseenres *glob_results, *glob_result;

#include "mini_httpd.h"
#include "templates.h"
#include "mini_httpd.c"
#include "templates.c"
#include "webseen_templates.c"
#include "wstcl.c"
#include "http_processing.c"

/* Calculate the memory we keep allocated.
 */
static int webseen_expmem()
{
  int size = 0;

  Context;
  size += expmem_templates();
  size += expmem_httpd();
  return size;
}

/* A report on the module status.
 *
 * details is either 0 or 1:
 *    0 - `.status'
 *    1 - `.status all'  or  `.module woobie'
 */
static void webseen_report(int idx, int details)
{
  int size;

  Context;
  size = webseen_expmem();
  if (details)
    dprintf(idx, "    using %d bytes\n", size);
}

static tcl_strings my_tcl_strings[] =
{
  {"webseen-log", httpd_log, 121, 0},
  {0, 0, 0, 0}
};

static char *webseen_close()
{
  Context;
  rem_tcl_commands(webseentcls);
  rem_tcl_strings(my_tcl_strings);
  unload_httpd();
  unload_templates();
  module_undepend(MODULE_NAME);
  return NULL;
}

/* Define the prototype here, to avoid warning messages in the
 * woobie_table.
 */
char *webseen_start();

/* This function table is exported and may be used by other modules and
 * the core.
 *
 * The first four have to be defined (you may define them as NULL), as
 * they are checked by eggdrop core.
 */
static Function webseen_table[] =
{
  (Function) webseen_start,
  (Function) webseen_close,
  (Function) webseen_expmem,
  (Function) webseen_report,
};

char *webseen_start(Function *global_funcs)
{
  /* Assign the core function table. After this point you use all normal
   * functions defined in src/mod/modules.h
   */
  global = global_funcs;


  Context;
  /* Register the module. */
  module_register(MODULE_NAME, webseen_table, 0, 0);
  /*                                            ^--- minor module version
   *                                         ^------ major module version
   *                           ^-------------------- module function table
   *              ^--------------------------------- module name
   */

  if (!module_depend(MODULE_NAME, "eggdrop", 104, 0)) {
	  if (!module_depend(MODULE_NAME, "eggdrop", 105, 0)) {
		  if (!module_depend(MODULE_NAME, "eggdrop", 106, 0)) {
			  if (!module_depend(MODULE_NAME, "eggdrop", 107, 0)) {
				  module_undepend(MODULE_NAME);
				  return "This module requires eggdrop1.5.0 or later";
			  }
		  }
	  }
  }
  if (!(gseen_funcs = module_depend(MODULE_NAME, "gseen", 1, 0))) {
    module_undepend(MODULE_NAME);
    return "You need the gseen module to use the webseen module.";
  }
  if (gseen_numversion < 10100) {
    module_undepend(MODULE_NAME);
    return "You need at least gseen v1.1.0 to run this module.";
  }
  add_tcl_commands(webseentcls);
  add_tcl_strings(my_tcl_strings);

  init_httpd();
  init_templates();
  start_httpd(8044);
  return NULL;
}
