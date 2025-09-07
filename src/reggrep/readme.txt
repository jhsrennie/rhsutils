GrepRegistry
============

GrepRegistry is a console mode search and replace tool for modifying
strings in the registry.  The syntax is:

  GrepRegistry [-lm -u -cr -cu -n -v[r] -i -q] <search> [<replace>]

  -lm search HKEY_LOCAL_MACHINE
  -u  search HKEY_USERS
  -cr search HKEY_CLASSES_ROOT (included in HKEY_LOCAL_MACHINE)
  -cu search HKEY_CURRENT_USER (included in HKEY_USERS)
  -n  search the key names
  -v  search the key values (default)
  -vr replace strings in the key values
  -i  search is not case sensitive
  -q  do not prompt when replacing

This looks a bit forbidding but it isn't really.  The four flags -lm,
-u, -cr and -cu control which of the four registry hives are searched.
You can specify any combination of these and if you don't specify any
the default is to search HKEY_CURRENT_USER only.

By default only values are searched, so if you type:

  grepregistry fred

this will find and values containing "fred" but not any key names.  To
search key names only specify the -n flag, and to search both key names
and key values specify -n and -v.  Specifying just -v is the same as
the default.  GrepRegistry will display exactly what it's searching for
and where.

Searches are case sensistive by default but you can use the -i flag to
make them case insensitive.

If you want to replace values in the registry specify the -vr flag.
You will be prompted to confirm each replacement, to supress the
prompts specify the -q flag.  You can only replace strings in values
and not in key names so there is no -nr flag.

GrepRegistry uses full regular expressions for searching and replacing,
so for example:

  grepregistry -i -vr ([cd]):\\windows \1:\\winnt

would replace "c:\windows" by "c:\winnt" and "d:\windows" by
"d:\winnt".  Note in particular the need to use \\ for the '\'
character because \ is an escape character in regular expressions.

At the moment GrepRegistry will only search and replace values of type
REG_SZ and REG_EXPAND_SZ.  It wouldn't be that hard to extend this to
REG_MULTI_SZ as well; I just haven't got around to it.  Obviously there
is no point in text searching numeric types, and I'm not sure how well
the regexp code would handle searching REG_BINARY data.

And finally the disclaimer.  I use this code myself, and so far haven't
suffered any ill efects as a result.  However I cannot guarantee that
it is bug free and you use it at your own risk.  Remember that editting
the registry is a potentially hazardous business and a mis-step can
destroy your NT installion.  Proceed with care!


John Rennie
jrennie@cix.compulink.co.uk
9th June 1997
