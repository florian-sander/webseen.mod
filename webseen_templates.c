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

#define TPL_UNDEFINED 0
#define TPL_NUMRESULTS 1
#define TPL_NICK 2
#define TPL_AGO 3
#define TPL_HOST 4
#define TPL_LOG 5
#define TPL_TIME 6
#define TPL_SEENERROR 7
#define TPL_QUERY 8
#define TPL_NUMSEENS 9
#define TPL_SEARCHTIME 10
#define TPL_TOTAL_QUERIES 11
#define TPL_TOTAL_SEARCHTIME 12
#define TPL_AVERAGE_SEARCHTIME 13

static void template_add_cmd_init_colorfade(struct tpl_content *, char *);
static void template_init_colorfade(int, struct tpl_content *);
static void template_send_fcolor(int, struct tpl_content *);
static void template_fade_color(int, struct tpl_content *);
static void template_send_misc(int, struct tpl_content *);
static void template_send_results(int, struct tpl_content *);
static void template_send_time(int, struct tpl_content *);
static void template_send_log(int, struct tpl_content *);
static void template_add_cmd_ifonchan(struct tpl_content *, char *);
static void template_send_ifonchan(int, struct tpl_content *);
static void template_add_cmd_results(struct tpl_content *, char *);

/* template_add_custom_command():
 * same as template_add_command() from templates.c, but for
 * non-general commands
 */
static int template_add_custom_command(struct tpl_content *tpc, char *command)
{
  char *s;

  if (!strncasecmp(command, "fcolor", 6)) {
    tpc->command = template_send_fcolor;
  } else if (!strncasecmp(command, "fade_color", 10)) {
    tpc->command = template_fade_color;
  } else if (!strncasecmp(command, "init_colorfade", 14)) {
    template_add_cmd_init_colorfade(tpc, command + 14);
  } else if (!strncasecmp(command, "results", 7)) {
    template_add_cmd_results(tpc, command + 8);
  } else if (!strncasecmp(command, "time", 4)) {
    tpc->command = template_send_time;
    s = command + 5;
    tpc->charpar1 = nmalloc(strlen(s) + 1);
    strcpy(tpc->charpar1, s);
  } else if (!strncasecmp(command, "numresults", 7)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_NUMRESULTS;
  } else if (!strncasecmp(command, "nick", 4)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_NICK;
  } else if (!strncasecmp(command, "ago", 3)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_AGO;
  } else if (!strncasecmp(command, "host", 4)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_HOST;
  } else if (!strncasecmp(command, "log", 3)) {
    tpc->command = template_send_log;
  } else if (!strncasecmp(command, "seenerror", 9)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_SEENERROR;
  } else if (!strncasecmp(command, "query", 5)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_QUERY;
  } else if (!strncasecmp(command, "numseens", 5)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_NUMSEENS;
  } else if (!strncasecmp(command, "searchtime", 11)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_SEARCHTIME;
  } else if (!strncasecmp(command, "total_queries", 13)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_TOTAL_QUERIES;
  } else if (!strncasecmp(command, "total_searchtime", 16)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_TOTAL_SEARCHTIME;
  } else if (!strncasecmp(command, "average_searchtime", 13)) {
    tpc->command = template_send_misc;
    tpc->what = TPL_AVERAGE_SEARCHTIME;
  } else if (!strncasecmp(command, "ifonchan", 8)) {
    template_add_cmd_ifonchan(tpc, command + 8);
  } else
    return 0;
  return 1;
}

/* template_send_misc():
 * outputs misc dynamic values like nicknames and hosts.
 */
static void template_send_misc(int idx, struct tpl_content *tpc)
{

  Context;
  // at first, check if the important variables are initialized and
  // output an error if they are not.
  if (!glob_result && glob_results)  // if at least glob_results exists, we can
    glob_result = glob_results;      // use its first entry as result...
  if (!glob_result && (tpc->what >= 2) && (tpc->what <= 5)) {
    putlog(LOG_MISC, "*", "WebSeen: ERROR: result uninitialized! (what = %d)", tpc->what);
    dprintf(idx, "<H1>ERROR! result uninitialized!");
    return;
  }
  if (tpc->what == TPL_NICK)
    dprintf(idx, "%s", glob_result->seen->nick);
  else if (tpc->what == TPL_AGO)
    dprintf(idx, "%s", gseen_duration(now - glob_result->seen->when));
  else if (tpc->what == TPL_HOST)
    dprintf(idx, "%s", glob_result->seen->host);
  else if (tpc->what == TPL_NUMRESULTS)
    dprintf(idx, "%d", numresults);
  else if (tpc->what == TPL_SEENERROR)
    dprintf(idx, "%s", getseenlang(920 + glob_seenerror));
  else if (tpc->what == TPL_QUERY)
    dprintf(idx, "%s", glob_query);
  else if (tpc->what == TPL_NUMSEENS)
    dprintf(idx, "%d", numseens);
  else if (tpc->what == TPL_SEARCHTIME)
    dprintf(idx, "%.3f", glob_searchtime);
  else if (tpc->what == TPL_TOTAL_QUERIES)
    dprintf(idx, "%d", glob_total_queries);
  else if (tpc->what == TPL_TOTAL_SEARCHTIME)
    dprintf(idx, "%.3f", glob_total_searchtime);
  else if (tpc->what == TPL_AVERAGE_SEARCHTIME)
    dprintf(idx, "%.3f", glob_total_searchtime / (float) glob_total_queries);
  else
    dprintf(idx, "<H1>ERROR! template_send_misc(): no directive to process %d"
            "</H1>", tpc->what);
}

/* template_send_time():
 * outputs the time when a user was last seen using the format which was
 * specified as parameter and stored in charpar1.
 */
static void template_send_time(int idx, struct tpl_content *tpc)
{
  char when[50];
  time_t tt;

  Context;
  // of course, we should check for unitialized variables, first
  if (!glob_result) {
    putlog(LOG_MISC, "*", "WebSeen: ERROR: result uninitialized! (what = %d)", tpc->what);
    dprintf(idx, "<H1>ERROR! result uninitialized!");
    return;
  }
  tt = glob_result->seen->when;
  // now use strftime to format the time according to the parameter
  strftime(when, 49, tpc->charpar1, localtime(&tt));
  // and finally output the time
  dprintf(idx, when);
}

/* template_send_log():
 * sends a reconstructed log by using the gseen language system
 */
static void template_send_log(int idx, struct tpl_content *tpc)
{
  char buf[256], *msg;

  sglobseendat = glob_result->seen;
  msg = buf;
  strncpy(buf, glob_result->seen->msg, 255);
  msg[255] = 0;
  sglobpunisher = newsplit(&msg);
  sglobreason = msg;
  dprintf(idx, "%s", getseenlang(900 + glob_result->seen->type));
}

/* template_add_cmd_results():
 * adds the command "results" to the template
 * "results" needs a sub-list with content which is sent for each result. This
 * sub-list may also contain commands
 */
static void template_add_cmd_results(struct tpl_content *h_tpc, char *params)
{
  Context;
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_results;
}

/* template_send_results():
 * loops through the results and outputs them via template_send_contents()
 */
static void template_send_results(int idx, struct tpl_content *h_tpc)
{
  gseenres *r;

  Context;
  for (r = glob_results; r; r = r->next) {
    glob_result = r;
    template_send_contents(idx, h_tpc->subcontent);
  }
}

/* template_add_cmd_ifonchan():
 * uses the same system as <?results?> this command outputs a line if the
 * queried user is still on the channel.
 */
static void template_add_cmd_ifonchan(struct tpl_content *h_tpc, char *params)
{
  Context;
  template_parse_content(NULL, h_tpc, params);
  h_tpc->command = template_send_ifonchan;
}

/* template_send_ifonchan():
 * see template_add_cmd_ifonchan() for description
 */
static void template_send_ifonchan(int idx, struct tpl_content *h_tpc)
{
  struct chanset_t *chan;
  memberlist *m;

  for (chan = chanset; chan; chan = chan->next) {
    m = ismember(chan, glob_results->seen->nick);
    if (m && !chan_issplit(m)) {
      template_send_contents(idx, h_tpc->subcontent);
      return;
    }
  }
}

/* template_add_cmd_init_colorfade():
 * Parameters: <startcolor> <endcolor> [steps]
 * initialiazes a color-fade from <startcolor> to <endcolor>
 * in [steps] steps. (if steps isn't defined, numresults is used)
 */
static void template_add_cmd_init_colorfade(struct tpl_content *h_tpc, char *params)
{
  char *startcolor, *endcolor, *steps;
  float istartcolor, iendcolor, isteps;

  Context;
  // remove leading spaces
  while (params[0] == ' ')
    params++;
  // get the parameters
  startcolor = newsplit(&params);
  endcolor = newsplit(&params);
  steps = newsplit(&params);
  istartcolor = strtol(startcolor, NULL, 0);
  iendcolor = strtol(endcolor, NULL, 0);
  isteps = strtol(steps, NULL, 0);
  // if there's no valid value for steps (strtol() returns 0 if it failed),
  // then just leave the 0. We'll replace it with numresults later

  // now write a pointer to the executing command and all parameters into our
  // content structure
  h_tpc->command = template_init_colorfade;
  h_tpc->floatpar1 = istartcolor;
  h_tpc->floatpar2 = iendcolor;
  h_tpc->intpar1 = isteps;
}

/* template_init_colorfade()
 * see template_add_cmd_init_colorfade for description
 */
static void template_init_colorfade(int idx, struct tpl_content *htpc)
{
  int wert, steps;
  float r2, b2, g2;

  // find out how many steps the color fade will have
  steps = htpc->intpar1;
  // if it has 0 steps, then the number of steps wasn't specified as parameter,
  // so we'll just use the number of results
  if (!steps)
    steps = numresults;
  // split our r/g/b values of the starting color (stored in intpar1)
  wert = htpc->floatpar1;
  glob_b = wert & 0xff; glob_g = (wert & 0xff00) >> 8; glob_r = (wert & 0xff0000) >> 16;
  // now do the same with the target color (intpar2)
  wert = htpc->floatpar2;
  b2 = wert & 0xff; g2 = (wert & 0xff00) >> 8; r2 = (wert & 0xff0000) >> 16;
  // finally, determine the "length" of a step between colors
  glob_rstep = (r2 - glob_r) / steps;
  glob_gstep = (g2 - glob_g) / steps;
  glob_bstep = (b2 - glob_b) / steps;
  // all global variables are now initialized and can be used.
}

/* template_send_fcolor():
 * outputs the current color-code
 */
static void template_send_fcolor(int idx, struct tpl_content *htpc)
{
  dprintf(idx, "#%02x%02x%02x", (int) glob_r, (int)  glob_g, (int) glob_b);
}

/* template_fade_color():
 * fades the color one step further
 */
static void template_fade_color(int idx, struct tpl_content *htpc)
{
  glob_r += glob_rstep;
  glob_g += glob_gstep;
  glob_b += glob_bstep;
}

