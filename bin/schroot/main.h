/* Copyright © 2005-2013  Roger Leigh <rleigh@codelibre.net>
 *
 * schroot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * schroot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *********************************************************************/

#ifndef SCHROOT_MAIN_H
#define SCHROOT_MAIN_H

#include <bin-common/main.h>
#include <bin/schroot/options.h>

#include <schroot/chroot/config.h>
#include <schroot/custom-error.h>

/**
 * schroot binary components.
 */
namespace bin
{
  /**
   * schroot program components.
   */
  namespace schroot
  {

    /**
     * Frontend for schroot.  This class is used to "run" schroot.
     */
    class main : public bin::common::main
    {
    public:
      /// Error codes.
      enum error_code
        {
          CHROOT_FILE,       ///< No chroots are defined in ....
          CHROOT_FILE2,      ///< No chroots are defined in ... or ....
          CHROOT_NOTDEFINED, ///< The specified chroots are not defined.
          SESSION_INVALID    ///< Invalid session name.
        };

      /// Exception type.
      typedef ::schroot::custom_error<error_code> error;

      typedef ::schroot::chroot::config::chroot_map chroot_map;

      /**
       * The constructor.
       *
       * @param options the command-line options to use.
       */
      main (options::ptr& options);

      /// The destructor.
      virtual ~main ();

      virtual void
      action_version (std::ostream& stream);

      /**
       * List chroots.
       */
      virtual void
      action_list ();

      /**
       * Print detailed information about chroots.
       */
      virtual void
      action_info ();

      /**
       * Print location of chroots.
       */
      virtual void
      action_location ();

      /**
       * Dump configuration file for chroots.
       */
      virtual void
      action_config ();

    protected:
      /**
       * Run the program.  This is the program-specific run method which
       * must be implemented in a derived class.
       *
       * @returns 0 on success, 1 on failure or the exit status of the
       * chroot command.
       */
      virtual int
      run_impl ();

      /**
       * Get a list of chroots based on the specified options (--all, --chroot).
       *
       * @returns a list of chroots.
       */
      void
      get_chroot_options ();

      /**
       * Load configuration.
       */
      virtual void
      load_config ();

      /**
       * Create a session.  This sets the session member.
       *
       * @param sess_op the session operation to perform.
       */
      virtual void
      create_session (::schroot::session::operation sess_op);

      /**
       * Add PAM authentication handler to the session.
       */
      virtual void
      add_session_auth ();

    private:
      /// The program options.
      options::ptr                    opts;
      /// The chroot configuration.
      ::schroot::chroot::config::ptr  config;
      /// The chroots to use (original names or aliases).
      ::schroot::string_list          chroot_names;
      /// The chroots to use (alias to chroot mapping).
      chroot_map                      chroots;
      /// The chroots to use (for session).
      ::schroot::session::chroot_list chroot_objects;
      /// The session.
      ::schroot::session::ptr         session;
    };

  }
}

#endif /* SCHROOT_MAIN_H */

/*
 * Local Variables:
 * mode:C++
 * End:
 */
