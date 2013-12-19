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

static int tcl_load_webseen_template STDVAR
{
  char *name, *file, *skin, *lang;

  Context;
  BADARGS(3, 5, " name file skin language");
  name = argv[1];
  file = argv[2];
  skin = argv[3];
  lang = argv[4];
  if (!loadtemplate(skin, lang, name, file)) {
    Tcl_AppendResult(irp, "Couldn't open template!!!", NULL);
    return TCL_ERROR;
  } else {
    return TCL_OK;
  }
}

static int tcl_add_webseen_desc STDVAR
{
  char *type, *name, *desc;

  Context;
  BADARGS(3, 4, " <skin/lang> <name> [description]");
  type = argv[1];
  if (!(!strcasecmp(type, "skin") || !strcasecmp(type, "lang"))) {
    Tcl_AppendResult(irp,
                     "Invalid argument. Must be either \"skin\" or \"lang\"",
                     NULL);
    return TCL_ERROR;
  }
  name = argv[2];
  if (argc > 3)
    desc = argv[3];
  else
    desc = name;
  add_description(type, name, desc);
  return TCL_OK;
}

static int tcl_set_webseen_default STDVAR
{
  char *type, *name;

  Context;
  BADARGS(3, 3, " <skin/lang> <name>");
  type = argv[1];
  if (!(!strcasecmp(type, "skin") || !strcasecmp(type, "lang"))) {
    Tcl_AppendResult(irp,
                     "Invalid argument. Must be either \"skin\" or \"lang\"",
                     NULL);
    return TCL_ERROR;
  }
  name = argv[2];
  if (!strcasecmp(type, "skin")) {
    default_skin = nrealloc(default_skin, strlen(name) + 1);
    strcpy(default_skin, name);
  } else {
    default_lang = nrealloc(default_lang, strlen(name) + 1);
    strcpy(default_lang, name);
  }
  return TCL_OK;
}

static tcl_cmds webseentcls[] =
{
  {"load_webseen_template", tcl_load_webseen_template},
  {"add_webseen_desc", tcl_add_webseen_desc},
  {"set_webseen_default", tcl_set_webseen_default},
  {0, 0}
};
