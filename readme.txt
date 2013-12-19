Description
-----------

Webseen.mod is an experimental addon to the eggdrop module gseen.mod. Like the
name already indicates, it creates an webinterface to gseen.mod. Just like
stats.mod, it starts its own webserver on a specified port (8044 by default).

It has support for multiple skins and languages.

ATTENTION: Please note that webseen.mod is far from ready from production use
and not actively maintained. It hasn't been updated for over a decade. It
probably will not be usable without adjustments. It requires a specific
version of gseen - which one exactly has not been passed down over the years.
The last known update to the code was on June 18th, 2001.

Installation
------------

Webseen.mod requires gseen.mod to run.

Just install it like every other module. (see gseen's readme for details)

Then copy the templates from the template/ subdir to your template
directory (~/eggdrop/templates, for example) and the langfiles to
your language directory, if they didn't already got copied there by the
installer.

Now copy webseen.conf to your eggdrop directory and edit it to fit your
needs. Finally, just add the line "source webseen.conf" to your eggdrop
config file.



Disclaimer:
----------

There is absolutely NO WARRANTY on this module. I do my best to make it work
properly, but if anything gets screwed up, I'm not responsible. Use this
module at your own risk.


About the author:
-----------------

Webseen.mod has been created by Florian Sander - http://www.kreativrauschen.de
