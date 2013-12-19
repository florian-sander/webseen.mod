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
#define TPL_MODULE_VERSION 14

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

  if (!strncasecmp(command, "results", 7)) {
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
  else if (tpc->what == TPL_MODULE_VERSION)
    dprintf(idx, "%s", MODULE_VERSION);
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
