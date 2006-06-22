/* Copyright © 2005-2006  Roger Leigh <rleigh@debian.org>
 *
 * schroot is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * schroot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 *********************************************************************/

#include <config.h>

#include <iostream>

#include <stdlib.h>

#include <boost/format.hpp>
#include <boost/program_options.hpp>

#include "sbuild.h"

#include "schroot-options.h"

using std::endl;
using boost::format;
namespace opt = boost::program_options;
using namespace schroot;

/*
 * schroot_options_new:
 *
 * Parse command-line options.
 *
 * Returns a structure containing the options.
 */
options::options (int   argc,
		  char *argv[]):
  action(ACTION_SESSION_AUTO),
  chroots(),
  command(),
  user(),
  preserve(false),
  quiet(false),
  verbose(false),
  all(false),
  all_chroots(false),
  all_sessions(false),
  session_force(false),
  compat(
#ifdef SBUILD_DCHROOT_DSA_COMPAT
	 COMPAT_DCHROOT_DSA
#elif defined(SBUILD_DCHROOT_COMPAT)
	 COMPAT_DCHROOT
#else
	 COMPAT_SCHROOT
#endif
	 )
{
  opt::options_description general(_("General options"));
  general.add_options()
    ("help,h",
     _("Show help options"))
    ("version,V",
     _("Print version information"))
    ("quiet,q",
     _("Show less output"))
    ("verbose,v",
     _("Show more output"))
    ("list,l",
     _("List available chroots"))
    ("info,i",
     _("Show information about selected chroots"));
    if (this->compat == COMPAT_SCHROOT)
      general.add_options()
	("location",
	 _("Print location of selected chroots"));
    else if (this->compat == COMPAT_DCHROOT)
      general.add_options()
	("path,p", opt::value<std::string>(&this->chroot_path),
	 _("Print path to selected chroot"));
    else if (this->compat == COMPAT_DCHROOT_DSA)
      general.add_options()
	("listpaths,p",
	 _("Print paths to available chroots"));
  general.add_options()
    ("config",
     _("Dump configuration of selected chroots"));

  opt::options_description chroot(_("Chroot selection"));
  chroot.add_options()
    ("chroot,c", opt::value<sbuild::string_list>(&this->chroots),
     _("Use specified chroot"));
  if (this->compat == COMPAT_SCHROOT)
    chroot.add_options()
      ("all,a",
       _("Select all chroots and active sessions"))
      ("all-chroots",
       _("Select all chroots"))
      ("all-sessions",
       _("Select all active sessions"));
  else
    chroot.add_options()
      ("all,a",
       _("Select all chroots"));

  opt::options_description chrootenv(_("Chroot environment"));
  if (this->compat == COMPAT_SCHROOT)
    chrootenv.add_options()
    ("user,u", opt::value<std::string>(&this->user),
     _("Username (default current user)"))
      ("preserve-environment,p",
       _("Preserve user environment"));
  else if (this->compat == COMPAT_DCHROOT)
    chrootenv.add_options()
      ("preserve-environment,d",
       _("Preserve user environment"));

  opt::options_description session(_("Session management"));
  session.add_options()
    ("begin-session,b",
     _("Begin a session; returns a session ID"))
    ("recover-session",
     _("Recover an existing session"))
    ("run-session,r",
     _("Run an existing session"))
    ("end-session,e",
     _("End an existing session"))
    ("force,f",
     _("Force operation, even if it fails"));

  opt::options_description hidden(_("Hidden options"));
  hidden.add_options()
    ("command", opt::value<sbuild::string_list>(&this->command),
     _("Command to run"))
    ("debug",
     _("Enable debugging messages"));

  opt::positional_options_description pos;
  pos.add("command", -1);

  opt::options_description visible;
  visible.add(general);
  visible.add(chroot);
  if (this->compat != COMPAT_DCHROOT_DSA)
    visible.add(chrootenv);
  if (this->compat == COMPAT_SCHROOT)
    visible.add(session);

  opt::options_description global;
  global.add(general);
  global.add(chroot);
  if (this->compat != COMPAT_DCHROOT_DSA)
    global.add(chrootenv);
  if (this->compat == COMPAT_SCHROOT)
    global.add(session);
  global.add(hidden);

  opt::variables_map vm;
  opt::store(opt::command_line_parser(argc, argv).
	     options(global).positional(pos).run(), vm);
  opt::notify(vm);

  if (vm.count("help"))
    {
      std::cout
	<< _("Usage:") << "\n  ";
      if (this->compat == COMPAT_SCHROOT)
	std::cout << "schroot";
      else if (this->compat == COMPAT_DCHROOT)
	std::cout << "dchroot";
      else if (this->compat == COMPAT_DCHROOT_DSA)
	std::cout << "dchroot-dsa";
      if (this->compat != COMPAT_DCHROOT_DSA)
	std::cout
	  << "  "
	  << _("[OPTION...] [COMMAND] - run command or shell in a chroot")
	  << '\n';
      else
	std::cout
	  << "  "
	  << _("[OPTION...] chroot [COMMAND] - run command or shell in a chroot")
	  << '\n';
      std::cout << visible << std::flush;
      exit(EXIT_SUCCESS);
    }

  if (vm.count("version"))
    set_action(ACTION_VERSION);
  if (vm.count("list"))
    set_action(ACTION_LIST);
  if (vm.count("info"))
    set_action(ACTION_INFO);
  if (vm.count("location") || vm.count("path") || vm.count("listpaths"))
    set_action(ACTION_LOCATION);
  if (vm.count("config"))
    set_action(ACTION_CONFIG);

  if (vm.count("all"))
    {
      if (this->compat == COMPAT_SCHROOT)
	this->all = true;
      else
	this->all_chroots = true;
    }
  if (vm.count("all-chroots"))
    this->all_chroots = true;
  if (vm.count("all-sessions"))
    this->all_sessions = true;

  if (vm.count("preserve-environment"))
    this->preserve = true;
  if (this->compat == COMPAT_DCHROOT_DSA)
    this->preserve = true;
  if (vm.count("quiet"))
    this->quiet = true;
  if (vm.count("verbose"))
    this->verbose = true;

  if (vm.count("begin-session"))
    set_action(ACTION_SESSION_BEGIN);
  if (vm.count("recover-session"))
    set_action(ACTION_SESSION_RECOVER);
  if (vm.count("run-session"))
    set_action(ACTION_SESSION_RUN);
  if (vm.count("end-session"))
    set_action(ACTION_SESSION_END);
  if (vm.count("force"))
    this->session_force = true;

  if (vm.count("debug"))
    sbuild::debug_level = sbuild::DEBUG_NOTICE;
  else
    sbuild::debug_level = sbuild::DEBUG_NONE;

  if (this->compat == COMPAT_DCHROOT_DSA)
    {
      // If no chroots specified, use the first non-option.
      if (this->chroots.empty() && !this->command.empty())
	{
	  this->chroots.push_back(this->command[0]);
	  this->command.erase(this->command.begin());
	}
    }

  // dchroot and dchroot-dsa only allow one command.
  if ((this->compat == COMPAT_DCHROOT ||
       this->compat == COMPAT_DCHROOT_DSA) &&
      this->command.size() > 1)
    throw opt::validation_error(_("Only one command may be specified"));

  if (this->quiet && this->verbose)
    {
      sbuild::log_warning()
	<< _("--quiet and --verbose may not be used at the same time")
	<< endl;
      sbuild::log_info() << _("Using verbose output") << endl;
    }

  if (!this->chroots.empty() && all_used())
    {
      sbuild::log_warning()
	<< _("--chroot and --all may not be used at the same time")
	<< endl;
      sbuild::log_info() << _("Using --chroots only") << endl;
      this->all = this->all_chroots = this->all_sessions = false;
    }

  if (this->all == true)
    {
      this->all_chroots = true;
      this->all_sessions = true;
    }

  /* Determine which chroots to load and use. */
  switch (this->action)
    {
    case ACTION_SESSION_AUTO:
      // Only allow normal chroots
      this->load_chroots = true;
      this->load_sessions = false;
      this->all = this->all_sessions = false;

      // If no chroot was specified, fall back to the "default" chroot.
      if (this->chroots.empty() && all_used() == false)
	this->chroots.push_back("default");

      break;
    case ACTION_SESSION_BEGIN:
      // Only allow one session chroot
      this->load_chroots = true;
      this->load_sessions = false;
      if (this->chroots.size() != 1 || all_used())
	throw opt::validation_error(_("Only one chroot may be specified when recovering, running or ending a session"));

      this->all = this->all_chroots = this->all_sessions = false;
      break;
    case ACTION_SESSION_RECOVER:
    case ACTION_SESSION_RUN:
    case ACTION_SESSION_END:
      // Session operations work on all chroots.
      this->load_chroots = this->load_sessions = true;
      break;
    case ACTION_VERSION:
      // Chroots don't make sense here.
      this->load_chroots = this->load_sessions = false;
      this->all = this->all_chroots = this->all_sessions = false;
      break;
    case ACTION_LIST:
      // If not specified otherwise, load normal chroots, but allow
      // --all options.
      if (!all_used())
	this->load_chroots = true;
      if (this->all_chroots)
	this->load_chroots = true;
      if (this->all_sessions)
	this->load_sessions = true;
      if (!this->chroots.empty())
	throw opt::validation_error(_("--chroot may not be used with --list"));
      break;
    case ACTION_INFO:
    case ACTION_LOCATION:
    case ACTION_CONFIG:
      // If not specified otherwise, load normal chroots, but allow
      // --all options.
      if (!this->chroots.empty()) // chroot specified
	this->load_chroots = this->load_sessions = true;
      else if (!all_used()) // no chroots specified
	{
	  this->all_chroots = true;
	  this->load_chroots = true;
	}
      if (this->all_chroots)
	this->load_chroots = true;
      if (this->all_sessions)
	this->load_sessions = true;
      break;
    default: // Something went wrong
      this->load_chroots = this->load_sessions = false;
      this->all = this->all_chroots = this->all_sessions = false;
      throw opt::validation_error(_("Unknown action specified"));
    }
}

options::~options ()
{
}

void
options::set_action (action_type action)
{
  if (this->action != ACTION_SESSION_AUTO)
    throw opt::validation_error(_("Only one action may be specified"));

  this->action = action;
}

/*
 * Local Variables:
 * mode:C++
 * End:
 */
