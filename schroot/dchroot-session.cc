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

#include "sbuild.h"

#include "dchroot-session.h"

#include <cassert>
#include <iostream>
#include <memory>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <syslog.h>

#include <boost/format.hpp>

#include <uuid/uuid.h>

using std::cout;
using std::endl;
using boost::format;
using namespace dchroot;

session::session (std::string const&         service,
		  config_ptr&                config,
		  operation                  operation,
		  sbuild::string_list const& chroots):
  sbuild::session(service, config, operation, chroots)
{
}

session::~session ()
{
}

sbuild::auth::status
session::get_chroot_auth_status (sbuild::auth::status status,
				 sbuild::chroot::ptr const& chroot) const
{
#ifndef SBUILD_DCHROOT_DSA_COMPAT
  status = change_auth(status, auth::STATUS_NONE);
#else
  /* DSA dchroot checks for a valid user in the groups list, unless
     the groups lists is empty in which case there are no
     restrictions.  This only applies if not switching users (dchroot
     does not support user switching) */

  sbuild::string_list const& users = chroot->get_users();
  sbuild::string_list const& groups = chroot->get_groups();

  if (this->get_ruid() == this->get_uid() &&
      users.empty() && groups.empty())
    status = change_auth(status, auth::STATUS_NONE);
  else
    status = change_auth(status,
			 sbuild::session::get_chroot_auth_status(status,
								 chroot));
#endif

  return status;
}

void
session::run_impl ()
{
  if (get_ruid() != get_uid())
    {
      format fmt(_("(%1%->%2%): dchroot sessions do not support user switching"));
      fmt % get_ruser().c_str() % get_user().c_str();
      throw error(fmt.str(), USER_SWITCH, _("dchroot session restriction"));
    }

  sbuild::session::run_impl();
}


/*
 * Local Variables:
 * mode:C++
 * End:
 */
