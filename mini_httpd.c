/*
 * Copyright (C) 2000,2001  Florian Sander
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

#define HTTPD_VERSION "1.1.0"

/* mini_httpd.c
 *
 * minimalistic http server for use in eggdrop modules
 *
 * Usage:
 *
 * add init_httpd() to module_start()
 * add unload_httpd() to module_close()
 * add expmem_httpd() to module_expmem()
 *
 * call start_httpd(port) to start listening for incoming connections
 * on the specified port.
 *
 * create a function "static void process_get_request(idx);". This function
 * gets called when someone connects to your server and sends an GET request.
 * you can access the requested path with http_connection(idx)->get.
 *
 * Don't forget to send the http header with send_http_header(int idx, code)
 * before you start sending the output.
 *
 * cookies are stored in http_connection(idx)->cookies, parameters in ->params.
 * You can also access them via get_cookie_value(idx, cookiename) or
 * get_param_value(idx, paramname).
 *
 * Variables (which you might want to tcl-trace and add to your config file)
 *
 * char httpd_ip[21] = "";
 *      Defines on which vhost httpd will listen for connections.
 *      If this is set to "", it'll listen on all vhosts.
 *
 * static char httpd_log[121] = "";
 *      Logfile to which http access will be loged (CLF format)
 *
 * static char httpd_loglevel[20] = "1";
 *      Defines to which loglevel access will be logged.
 *
 * static char httpd_ignore_msg[256] = "<H1>You are on ignore.</H1>";
 *      reply which the recipient will see, if he/she is on ignore
 *
 * static int max_http_thr = 0;
 * static int max_http_time = 0;
 *      simple flood protection. Allows only thr connections in time seconds.
 *
 */

static void process_get_request(int);

static char httpd_ip[21] = "";
static char httpd_loglevel[20] = "1";
static char httpd_ignore_msg[256] = "<H1>You are on ignore.</H1>";
static char httpd_log[121] = "";
static int max_http_thr = 0;
static int max_http_time = 0;
static int http_timeout = 2;
static int httpd_dcc_index = -1;

char *decoded_url;

struct cookielist {
  struct cookielist *next;
  char *name;
  char *value;
};

struct paramlist {
  struct paramlist *next;
  char *param;
  char *value;
};

struct http_connection_data {
  int traffic;
  int code;
  char *browser;
  char *referer;
  char *get;
  char *cmd;
  struct cookielist *cookies;
  struct paramlist *params;
};

static void init_httpd();
static int expmem_httpd();
static void unload_httpd();
static void start_httpd(int);
static void stop_httpd();
static void init_http_connection_data(int);
static void free_http_connection_data(int);
static void http_activity(int, char *, int);
static void send_http_header(int, int);
static void add_cookies(int, char *);
static int expmem_cookies(struct cookielist *);
static void free_cookies(struct cookielist *);
static char *get_cookie_value(int, char *);
static void add_params(int, char *);
static int expmem_params(struct paramlist *);
static void free_params(struct paramlist *);
static char *get_param_value(int, char *);
#ifndef OLDBOT
static void outdone_http(int);
#endif
static void display_http(int, char *);
static void display_httpd_accept(int, char *);
static void timeout_http(int);
static void timeout_listen_httpd(int);
static void kill_http(int, void *);
static int expmem_http(void *);
static void out_http(int, char *, void *);
static void httpd_accept(int, char *, int);
static int http_flood();
static void eof_http(int);
static char *decode_url(char *);
static char *csplit(char **, char);

static struct dcc_table MHTTPD_CON_HTTPD =
{
  "HTTPD",
  DCT_VALIDIDX,
  eof_http,
  httpd_accept,
  0,
  timeout_listen_httpd,
  display_httpd_accept,
  0,
  NULL,
  0
};

static struct dcc_table MHTTPD_CON_HTTP =
{
  "HTTP",
  DCT_VALIDIDX,
  eof_http,
  http_activity,
  &http_timeout,
  timeout_http,
  display_http,
  expmem_http,
  kill_http,
#ifdef OLDBOT
  out_http,
#else
  out_http,
  outdone_http
#endif
};

#define http_connection(i) ((struct http_connection_data *) dcc[(i)].u.other)

char *decoded_url = NULL;

/* init_httpd()
 * initializes a few variables
 */
static void init_httpd()
{
  decoded_url = NULL;
}

/* expmem_httpd()
 * expmem function
 */
static int expmem_httpd()
{
  int size = 0;

  if (decoded_url)
    size += strlen(decoded_url) + 1;
  return size;
}

/* unload_httpd():
 * frees all allocated memory, stops listening and kills all
 * existing connections.
 */
static void unload_httpd()
{
  stop_httpd();
  if (decoded_url)
    nfree(decoded_url);
}

/* start_httpd():
 * starts listening for http connections on the defined port.
 */
static void start_httpd(int port)
{
  int i, zz;
  char tmp[50];

  Context;
  // a little hack to make httpd listen on the defined vhost
  // (or on all vhosts, if none is defined)
  // Just set my-ip to the wanted vhost, since open_listen()
  // uses this var
  sprintf(tmp, "set my-ip \"%s\";", httpd_ip);
  do_tcl("httpd-hack-start",
      "set my-ip-httpd-backup ${my-ip};"
      "set my-hostname-httpd-backup ${my-hostname};"
      "set my-hostname \"\"");
  do_tcl("httpd-hack-setip", tmp);
  // now get a listening socket
  zz = open_listen(&port);
  // don't forget to restore my-ip when we're done ^_^
  do_tcl("httpd-hack-end",
      "set my-ip ${my-ip-httpd-backup};"
      "set my-hostname ${my-hostname-httpd-backup}");
  // ohoh, we didn't get a socket :(
  if (zz == (-1)) {
    putlog(LOG_MISC, "*", "ERROR! Cannot open listening socket for httpd!");
    return;
  }
  // now add this new socket to our dcc table and display a warning,
  // if the table is full
  if ((i = new_dcc(&MHTTPD_CON_HTTPD, 0)) == -1) {
    putlog(LOG_MISC, "*", "ERROR! Cannot open listening socket for httpd! DCC table is full!");
    // better kill the socket, before we get a "phantom-socket" ^_^
    killsock(zz);
    return;
  }
  // store the index in a global var, so we can access it easily later...
  httpd_dcc_index = i;
  // now fill the dcc-struct with information
  dcc[i].sock = zz;
  dcc[i].addr = (IP) (-559026163);
  dcc[i].port = port;
  strcpy(dcc[i].nick, "httpd");
  strcpy(dcc[i].host, "*");
  dcc[i].timeval = now;
  putlog(LOG_MISC, "*", "Now listening for http connections on port %d", port);
}

/* stop_httpd()
 * kills all httpd connections and listening sockets
 */
static void stop_httpd()
{
  int i;

  for (i = 0; i < dcc_total; i++) {
    if (dcc[i].type == &MHTTPD_CON_HTTPD) {
      putlog(LOG_MISC, "*",
      	     "no longer listening for http connections on port %d",
             dcc[i].port);
      killsock(dcc[i].sock);
      lostdcc(i);
    } else if (dcc[i].type == &MHTTPD_CON_HTTP) {
      putlog(LOG_MISC, "*", "killing http connection from %s", dcc[i].host);
      killsock(dcc[i].sock);
      lostdcc(i);
    }
  }
}

/* init_http_connection_data():
 * inits all variables in our http_connection_data struct
 */
static void init_http_connection_data(int idx)
{
  http_connection(idx)->traffic = 0;
  http_connection(idx)->code = -1;
  http_connection(idx)->browser = NULL;
  http_connection(idx)->referer = NULL;
  http_connection(idx)->get = NULL;
  http_connection(idx)->cmd = NULL;
  http_connection(idx)->cookies = NULL;
  http_connection(idx)->params = NULL;
}

/* free_http_connection_data():
 * frees all data of our http_connection_data struct
 */
static void free_http_connection_data(int idx)
{
  if (http_connection(idx)->browser)
    nfree(http_connection(idx)->browser);
  if (http_connection(idx)->referer)
    nfree(http_connection(idx)->referer);
  if (http_connection(idx)->get)
    nfree(http_connection(idx)->get);
  if (http_connection(idx)->cmd)
    nfree(http_connection(idx)->cmd);
  if (http_connection(idx)->cookies)
    free_cookies(http_connection(idx)->cookies);
  if (http_connection(idx)->params)
    free_params(http_connection(idx)->params);
  nfree(http_connection(idx));
}

/* http_activity():
 * handles all the data that the browser sends to us.
 */
static void http_activity(int idx, char *buf, int len)
{
  char *cmd, *path, *imask;
  int lev;

  debug2("%s: %s", dcc[idx].host, buf);
  // at first, check if the user is on ignore and therefore should
  // be ignored
  imask = nmalloc(strlen(dcc[idx].host) + 11);
  sprintf(imask, "http!*@%s", dcc[idx].host);
  if (match_ignore(imask)) {
    debug1("Ignoring http access from %s", dcc[idx].host);
    send_http_header(idx, 401);
    if (httpd_ignore_msg[0])
      dprintf(idx, "%s", httpd_ignore_msg);
    killsock(dcc[idx].sock);
    lostdcc(idx);
    nfree(imask);
    return;
  }
  nfree(imask);
  // at first, check for recognized commands which we want to be logged
  // (only GET is supported, at the moment)
  if (!strncasecmp(buf, "GET ", 4) && !http_connection(idx)->cmd) {
    http_connection(idx)->cmd = nmalloc(strlen(buf) + 1);
    strcpy(http_connection(idx)->cmd, buf);
  }
  // now check if we know the command and store all info that we need
  cmd = newsplit(&buf);
  // GET: request for a document
  if (!strcasecmp(cmd, "GET")) {
    // at first, check if we're being flooded and kill the connection
    // if there were too many requests.
    if (http_flood()) {
      http_connection(idx)->code = 401;
      killsock(dcc[idx].sock);
      lostdcc(idx);
      return;
    }
    // now log the access
    lev = logmodes(httpd_loglevel);
    if (lev)
      putlog(lev, "*", "%s: GET %s", dcc[idx].host, buf);
    // and finally store the request, if there wasn't already one before.
    if (!http_connection(idx)->get) {
      path = newsplit(&buf);
      // cut the parameters off and store them
      add_params(idx, path);
      http_connection(idx)->get = nmalloc(strlen(path) + 1);
      strcpy(http_connection(idx)->get, path);
    }
  // user-agent: browser-information
  } else if (!strcasecmp(cmd, "User-Agent:")) {
    if (http_connection(idx)->browser)
      return;
    http_connection(idx)->browser = nmalloc(strlen(buf) + 1);
    strcpy(http_connection(idx)->browser, buf);
  } else if (!strcasecmp(cmd, "Referer:")) {
    if (http_connection(idx)->referer)
      return;
    http_connection(idx)->referer = nmalloc(strlen(buf) + 1);
    strcpy(http_connection(idx)->referer, buf);
  } else if (!strcasecmp(cmd, "Cookie:")) {
    add_cookies(idx, buf);
  } else if (!buf[0]) {
    process_get_request(idx);
    dcc[idx].status = 1;
#ifndef OLDBOT
    /* If there's no data in our socket, we immediately get rid of it.
     */
    if (!sock_has_data(SOCK_DATA_OUTGOING, dcc[idx].sock)) {
      killsock(dcc[idx].sock);
      lostdcc(idx);
    }
#endif
  }
}

/* send_http_header()
 * sends the http header
 */
static void send_http_header(int idx, int code)
{
  if (code == 200)
    dprintf(idx, "HTTP/1.0 200 OK\n");
  else if (code == 401)
    dprintf(idx, "HTTP/1.0 401 Access Forbidden\n");
  else if (code == 404)
    dprintf(idx, "HTTP/1.1 404 Not Found\n");
  else
    dprintf(idx, "HTTP/1.0 %d\n", code);
  dprintf(idx, "Server: EggdropMiniHTTPd/%s\n", HTTPD_VERSION);
  dprintf(idx, "Content-Type: text/html\n");
  dprintf(idx, "\n");
  http_connection(idx)->code = code;
}

/* add_cookies()
 * simple function to add one or more cookies to the cookie-list
 */
static void add_cookies(int idx, char *buf)
{
  struct cookielist *nc;
  char *cookie, *name, *value;

  while (buf[0]) {
    cookie = csplit(&buf, ';');
    while (cookie[0] == ' ')
      cookie++;
    name = csplit(&cookie, '=');
    value = cookie;
    nc = nmalloc(sizeof(struct cookielist));
    nc->next = NULL;
    nc->name = nmalloc(strlen(name) + 1);
    strcpy(nc->name, name);
    nc->value = nmalloc(strlen(value) + 1);
    strcpy(nc->value, value);
    nc->next = http_connection(idx)->cookies;
    http_connection(idx)->cookies = nc;
  }
}

/* expmem_cookies()
 * simple expmem function for a cookie list
 */
static int expmem_cookies(struct cookielist *c)
{
  int size = 0;

  for (; c; c = c->next) {
    size += sizeof(struct cookielist);
    size += strlen(c->name) + 1;
    size += strlen(c->value) + 1;
  }
  return size;
}

/* free_cookies()
 * simple function to free a cookie list (surprise!)
 */
static void free_cookies(struct cookielist *c)
{
  struct cookielist *next;

  while (c) {
    next = c->next;
    nfree(c->name);
    nfree(c->value);
    nfree(c);
    c = next;
  }
}

static char *get_cookie_value(int idx, char *name)
{
  struct cookielist *cookie;

  Assert(idx >= 0);
  for (cookie = http_connection(idx)->cookies; cookie; cookie = cookie->next)
    if (!strcasecmp(cookie->name, name))
      return cookie->value;
  return NULL;
}

/* add_params():
 * extracts all parameters from the URL and stores them
 * in a simple linked list
 */
static void add_params(int idx, char *buf)
{
  char *param, *name, *value;
  struct paramlist *p;

  csplit(&buf, '?');
  while (buf[0]) {
    param = csplit(&buf, '&');
    name = csplit(&param, '=');
    value = decode_url(param);
    p = nmalloc(sizeof(struct paramlist));
    p->param = nmalloc(strlen(name) + 1);
    strcpy(p->param, name);
    p->value = nmalloc(strlen(value) + 1);
    strcpy(p->value, value);
    p->next = http_connection(idx)->params;
    http_connection(idx)->params = p;
  }
}

static int expmem_params(struct paramlist *p)
{
  int size = 0;

  for (; p; p = p->next) {
    size += sizeof(struct paramlist);
    size += strlen(p->param) + 1;
    size += strlen(p->value) + 1;
  }
  return size;
}

static void free_params(struct paramlist *p)
{
  struct paramlist *next;

  while (p) {
    next = p->next;
    nfree(p->param);
    nfree(p->value);
    nfree(p);
    p = next;
  }
}

static char *get_param_value(int idx, char *name)
{
  struct paramlist *param;

  Assert(idx >= 0);
  for (param = http_connection(idx)->params; param; param = param->next)
    if (!strcasecmp(param->param, name))
      return param->value;
  return NULL;
}

#ifndef OLDBOT
static void outdone_http(int idx)
{
  if (dcc[idx].status) {
    killsock(dcc[idx].sock);
    lostdcc(idx);
  } else
    dcc[idx].status = 1;
}
#endif

static void display_http(int idx, char *buf)
{
  sprintf(buf, "http connection");
}

static void display_httpd_accept(int idx, char *buf)
{
  sprintf(buf, "httpd");
}

static void timeout_http(int idx)
{
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

static void timeout_listen_httpd(int idx)
{
  debug0("timeout httpd listen");
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

/* kill_http():
 * This function called when connection is killed. It
 * logs the connection to the logfile, if one exists.
 */
static void kill_http(int idx, void *x)
{
  char ts[41], test[11];
  time_t tt;
  FILE *f;

  Context;
  tt = now;
  ctime(&tt);
  /* 05/Feb/2000:12:08:17 +0100 */
  strftime(test, 19, "%z", localtime(&tt));
  // if test contains 'z' then strftime() doesn't support
  // %z (timezone-offset) on this system, and we have to
  // use a config var instead
  if (test[0] != 'z')
    strftime(ts, 40, "%d/%b/%Y:%H:%M:%S %z", localtime(&tt));
  else
    strftime(ts, 40, "%d/%b/%Y:%H:%M:%S", localtime(&tt));
  if (httpd_log[0]) {
    f = fopen(httpd_log, "a");
    if (f == NULL)
      putlog(LOG_MISC, "*", "ERROR writing httpd log.");
    else {
      if (test[0] != 'z')
        fprintf(f,
         "%s - - [%s] \"%s\" %d %d \"%s\" \"%s\"\n", dcc[idx].host, ts,
         http_connection(idx)->cmd ? http_connection(idx)->cmd : "",
         http_connection(idx)->code, http_connection(idx)->traffic,
         http_connection(idx)->referer ? http_connection(idx)->referer : "-",
         http_connection(idx)->browser ? http_connection(idx)->browser : "");
      else
        fprintf(f,
         "%s - - [%s %+05d] \"%s\" %d %d \"%s\" \"%s\"\n",
         dcc[idx].host, ts, offset * (-1) * 100,
         http_connection(idx)->cmd ? http_connection(idx)->cmd : "",
         http_connection(idx)->code, http_connection(idx)->traffic,
         http_connection(idx)->referer ? http_connection(idx)->referer : "-",
         http_connection(idx)->browser ? http_connection(idx)->browser : "");
      fclose(f);
    }
  }
  // don't forget to free the data when we're done.
  free_http_connection_data(idx);
}

/* expmem_http()
 * expmem's all data allocated to store browser info, referer, cookies, etc...
 */
static int expmem_http(void *x)
{
  register struct http_connection_data *p = (struct http_connection_data *) x;
  int tot = 0;

  Context;
  if (!p) {
    putlog(LOG_MISC, "*", "Can't expmem clientinfo, no pointer. This should not happen!");
    return 0;
  }
  tot += sizeof(struct http_connection_data);
  if (p->browser)
    tot += strlen(p->browser) + 1;
  if (p->referer)
    tot += strlen(p->referer) + 1;
  if (p->get)
    tot += strlen(p->get) + 1;
  if (p->cmd)
    tot += strlen(p->cmd) + 1;
  if (p->cookies)
    tot += expmem_cookies(p->cookies);
  if (p->params)
    tot += expmem_params(p->params);
  return tot;
}

/* out_http():
 * Called when data is sent through the socket. Used to log the
 * sent traffic.
 */
static void out_http(int idx, char *buf, void *x)
{
  register struct http_connection_data *p = (struct http_connection_data *) x;

  if (!p) {
    putlog(LOG_MISC, "*", "No http_connection pointer. This should not happen!");
    return;
  }
  p->traffic += strlen(buf);
  tputs(dcc[idx].sock, buf, strlen(buf));
}

/* http_accept():
 * accepts an incoming http connection
 */
static void httpd_accept(int idx, char *buf, int len)
{
  unsigned long ip;
  unsigned short port;
  int j = 0, sock, i;
  char s[UHOSTLEN];

  Context;
  if (dcc_total + 1 >= max_dcc) {
    j = answer(dcc[idx].sock, s, &ip, &port, 0);
    if (j != -1) {
      dprintf(-j, "Sorry, too many connections already.\r\n");
      killsock(j);
    }
    return;
  }
  sock = answer(dcc[idx].sock, s, &ip, &port, 0);
  if (sock < 0) {
    neterror(s);
    putlog(LOG_MISC, "*", "HTTPd: Error accepting connection: %s", s);
    return;
  }
  if ((i = new_dcc(&MHTTPD_CON_HTTP, sizeof(struct http_connection_data))) == (-1)) {
    putlog(LOG_MISC, "*", "Error accepting http connection. DCC table is full.");
    killsock(sock);
    return;
  }
  dcc[i].sock = sock;
  dcc[i].addr = ip;
  dcc[i].port = port;
  strcpy(dcc[i].nick, "http");
#ifndef OLDBOT
  sprintf(s, "%s", iptostr(my_htonl(ip)));
#else
  sprintf(s, "%lu.%lu.%lu.%lu", (ip >> 24) & 0xff, (ip >> 16) & 0xff,
  	  (ip >> 8) & 0xff, ip & 0xff); /* dw */
#endif
  strcpy(dcc[i].host, s);
  dcc[i].timeval = now;
  dcc[i].status = 0;
  // init http_connection_data struct
  init_http_connection_data(i);
}

/* http_flood()
 * simple check for floods
 */
static int mhttp_time = 0, mhttp_thr = 0;
static int http_flood()
{
  if (!max_http_thr || !max_http_time)
    return 0;
  if ((now - mhttp_time) > max_http_time) {
    mhttp_time = now;
    mhttp_thr = 0;
  }
  mhttp_thr++;
  if (mhttp_thr > max_http_thr)
    return 1;
  return 0;
}

static void eof_http(int idx)
{
  debug0("eof http");
  killsock(dcc[idx].sock);
  lostdcc(idx);
}

/* decode_url():
 * Decodes all special characters(%3F == '?', %21 == '!', etc)
 * and returns the decoded url
 */
static char *decode_url(char *paramurl)
{
  char *p, *buf, *url, c, hex[5];
  long int i;

  Context;
  // free url-buffer (global var)
  if (decoded_url)
    nfree(decoded_url);
  // initialize url-buffer
  decoded_url = nmalloc(1);
  decoded_url[0] = 0;
  // copy parameter into a buffer
  buf = nmalloc(strlen(paramurl) + 1);
  strcpy(buf, paramurl);
  url = buf;
  // now loop to get every encoded char
  while ((p = strchr(url, '%'))) {
    // '%' marks the beginning of an encoded char, so cut the string here.
    p[0] = 0;
    // append the first part of the string to our buffer
    decoded_url = nrealloc(decoded_url, strlen(decoded_url) + strlen(url) + 1);
    strcat(decoded_url, url);
    // set the pointer to the beginning of the next string
    url = p + 1;
    // first check if there are enough chars left to decode
    if (strlen(url+1) >= 2) {
      // the number behind '%' is a hex-number which is the ASCII code of
      // the char, so dump the hex into a string and scan it
      snprintf(hex, 5, "0x%c%c", p[1], p[2]);
      i = strtol(hex, NULL, 0);
      if (!i) {
        i = '?';
        debug0("MiniHTTPd: decode_url(): i is 0");
      }
      c = (char) i;
      // now append the decoded char to the url
      decoded_url = nrealloc(decoded_url, strlen(decoded_url) + 1 + 1);
      sprintf(decoded_url, "%s%c", decoded_url, c);
      // increase the pointer to abandon the encoded char
      url += 2;
    } else {
      // just append the original '%' if there weren't enough chars to decode
      decoded_url = nrealloc(decoded_url, strlen(decoded_url) + 1 + 1);
      strcat(decoded_url, "%");
    }
  }
  // finally append the rest of the param to our buffer. There are no encoded
  // chars left.
  decoded_url = nrealloc(decoded_url, strlen(decoded_url) + strlen(url) + 1);
  strcat(decoded_url, url);
  // free the buffer
  nfree(buf);
  Context;
  return decoded_url;
}

/* csplit()
 * basically the same as nsplit, but allows you to define
 * the divider.
 */
static char *csplit(char **rest, char divider)
{
  register char *o, *r;

  if (!rest)
    return *rest = "";
  o = *rest;
  while (*o == divider)
    o++;
  r = o;
  while (*o && (*o != divider))
    o++;
  if (*o)
    *o++ = 0;
  *rest = o;
  return r;
}