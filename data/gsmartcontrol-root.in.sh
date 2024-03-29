#!/bin/bash
###############################################################################
# License: Zlib
# Copyright:
#   (C) 2008 - 2022 Alexander Shaduri <ashaduri@gmail.com>
###############################################################################

# Run gsmartcontrol with root, asking for root password first.
# export GSMARTCONTROL_SU to override a su command (e.g. "kdesu -c").

EXEC_BIN="@CMAKE_INSTALL_FULL_SBINDIR@/gsmartcontrol";
program_name="gsmartcontrol"


# Preserve quotes in arguments
final_args_quoted="";
for i in "$@";do
    final_args_quoted="$final_args_quoted \"${i//\"/\\\"}\"";
done;


DESKTOP="auto";

# Compatibility with old syntax:
# gsmartcontrol-root [<desktop> [program_options]]
if [ "$1" == "auto" ] || [ "$1" == "kde" ] || [ "$1" == "gnome" ] || [ "$1" == "other" ]; then
	DESKTOP="$1";
	shift;  # remove $1
else
	# New syntax:
	# gsmartcontrol-root [--desktop=<auto|kde|gnome|other>] [program_options]

	for arg in "$@"; do
		case $arg in
			--desktop=*)
				DESKTOP="${arg#*=}";
				final_args_quoted="${final_args_quoted/\"$arg\"/}";
				;;
			*)
				# unknown option
				;;
		esac
	done
fi

if [ "$DESKTOP" != "auto" ] && [ "$DESKTOP" != "kde" ] && \
		[ "$DESKTOP" != "gnome" ] && [ "$DESKTOP" != "other" ]; then
	echo "Usage: $0 [--desktop=<auto|kde|gnome|other>] [<${program_name}_options>]";
	exit 1;
fi


# Auto-detect current desktop if auto was specified.
if [ "$DESKTOP" = "auto" ]; then
	# KDE_SESSION_UID is present on kde3 and kde4.
	# Note that it may be empty (but still set)
	if [ "${KDE_SESSION_UID+set}" = "set" ]; then
		DESKTOP="kde";
	# same with gnome
	elif [ "${GNOME_DESKTOP_SESSION_ID+set}" = "set" ]; then
		DESKTOP="gnome";
	else
		DESKTOP="other";
	fi
fi

# echo $DESKTOP;





# They're basically the same, only the order is different.
# pkexec is for PolKit.
# sux requires xterm to ask for the password.
# xdg-su is basically like this script, except worse :)
# su-to-root is a debian/ubuntu official method (although gksu is available).
gnome_sus="pkexec su-to-root gnomesu gksu kdesu beesu xdg-su sux";
kde_sus="pkexec su-to-root kdesu gnomesu gksu beesu xdg-su sux";
other_sus="$gnome_sus";


candidates="";
found_su=""

if [ "$DESKTOP" = "gnome" ]; then
	candidates="$gnome_sus";
elif [ "$DESKTOP" = "kde" ]; then
	candidates="$kde_sus";
elif [ "$DESKTOP" = "other" ]; then
	candidates="$other_sus";
fi

if [ "$GSMARTCONTROL_SU" = "" ]; then
	for subin in $candidates; do
		which "$subin" &>/dev/null
		if [ $? -eq 0 ]; then
			found_su="$subin";
			break;
		fi
	done

	if [ "$found_su" = "" ]; then
		xmessage "Error launching ${program_name}: No suitable su mechanism found.
Try installing PolKit, kdesu, gnomesu, gksu, beesu or sux first.";
		exit 1;
	fi
fi


# gnomesu and gksu (but not kdesu, not sure about others) fail to adopt
# root's PATH. Since the user's PATH may not contain /usr/sbin (with smartctl)
# on some distributions (e.g. mandriva), add it manually. We also add
# /usr/local/sbin, since that's the default location of custom-compiled smartctl.
# Add these directories _before_ existing PATH, so that the user is not
# tricked into running it from some other path (we're running as root with
# the user's env after all).
# Add sbindir as well (freebsd seems to require it).
# Note that beesu won't show a GUI login box if /usr/sbin is before /usr/bin,
# so add it first as well.
EXTRA_PATHS="/usr/bin:/usr/sbin:/usr/local/sbin:@CMAKE_INSTALL_FULL_SBINDIR@";
export PATH="$EXTRA_PATHS:$PATH"


# echo $found_su;

# Examples:
# gnomesu -c 'gsmartcontrol --no-scan'
# kdesu -c 'gsmartcontrol --no-scan'
# beesu -P 'gsmartcontrol --no-scan'
# su-to-root -X -c 'gsmartcontrol --no-scan'
# xterm -e sux -c 'gsmartcontrol --no-scan'  # sux asks for password in a terminal


full_cmd="";

if [ "$GSMARTCONTROL_SU" != "" ]; then
	full_cmd="$GSMARTCONTROL_SU '$EXEC_BIN $final_args_quoted'";

elif [ "$found_su" = "pkexec" ]; then
	if [ "$GDK_SCALE" != "" ]; then
		final_args_quoted="$final_args_quoted --gdk-scale='$GDK_SCALE'"
	fi
	if [ "$GDK_DPI_SCALE" != "" ]; then
		final_args_quoted="$final_args_quoted --gdk-dpi-scale='$GDK_DPI_SCALE'"
	fi
	full_cmd="pkexec --disable-internal-agent $EXEC_BIN $final_args_quoted";

elif [ "$found_su" = "sux" ]; then
	full_cmd="xterm -e sux -c '$EXEC_BIN $final_args_quoted'";

elif [ "$found_su" = "gksu" ]; then
	full_cmd="$found_su '$EXEC_BIN $final_args_quoted'";

elif [ "$found_su" = "beesu" ]; then
	full_cmd="$found_su -P '$EXEC_BIN $final_args_quoted'";

elif [ "$found_su" = "su-to-root" ]; then
	full_cmd="$found_su -X -c '$EXEC_BIN $final_args_quoted'";

else  # gnomesu, kdesu, xdg-su
	full_cmd="$found_su -c '$EXEC_BIN $final_args_quoted'";
fi


# echo $full_cmd
eval "$full_cmd"

