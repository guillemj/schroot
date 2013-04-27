/* Copyright © 2005-2013  Roger Leigh <rleigh@debian.org>
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

#ifndef SBUILD_CHROOT_FACET_LVM_SNAPSHOT_H
#define SBUILD_CHROOT_FACET_LVM_SNAPSHOT_H

#include <sbuild/chroot/facet/block-device-base.h>

namespace sbuild
{
  namespace chroot
  {
    namespace facet
    {

      /**
       * A chroot stored on an LVM logical volume (LV).
       *
       * A snapshot LV will be created and mounted on demand.
       */
      class lvm_snapshot : public block_device_base
      {
      public:
        /// A shared_ptr to a chroot facet object.
        typedef std::shared_ptr<lvm_snapshot> ptr;

        /// A shared_ptr to a const chroot facet object.
        typedef std::shared_ptr<const lvm_snapshot> const_ptr;

      protected:
        /// The constructor.
        lvm_snapshot ();

        /// The copy constructor.
        lvm_snapshot (const lvm_snapshot& rhs);

        void
        set_chroot (chroot& chroot);

        friend class chroot;

      public:
        /// The destructor.
        virtual ~lvm_snapshot ();

        virtual std::string const&
        get_name () const;

        /**
         * Create a chroot facet.
         *
         * @returns a shared_ptr to the new chroot facet.
         */
        static ptr
        create ();

        facet::ptr
        clone () const;

        /**
         * Get the logical volume snapshot device name.  This is used by
         * lvcreate.
         *
         * @returns the device name.
         */
        std::string const&
        get_snapshot_device () const;

        /**
         * Set the logical volume snapshot device name.  This is used by
         * lvcreate.
         *
         * @param snapshot_device the device name.
         */
        void
        set_snapshot_device (std::string const& snapshot_device);

        /**
         * Get the logical volume snapshot options.  These are used by
         * lvcreate.
         *
         * @returns the options.
         */
        std::string const&
        get_snapshot_options () const;

        /**
         * Set the logical volume snapshot options.  These are used by
         * lvcreate.
         *
         * @param snapshot_options the options.
         */
        void
        set_snapshot_options (std::string const& snapshot_options);

        virtual void
        setup_env (chroot const& chroot,
                   environment&  env) const;

        virtual chroot::session_flags
        get_session_flags (chroot const& chroot) const;

      protected:
        virtual void
        setup_lock (chroot::setup_type type,
                    bool               lock,
                    int                status);

        virtual void
        get_details (chroot const& chroot,
                     format_detail& detail) const;

        virtual void
        get_used_keys (string_list& used_keys) const;

        virtual void
        get_keyfile (chroot const& chroot,
                     keyfile& keyfile) const;

        virtual void
        set_keyfile (chroot&        chroot,
                     keyfile const& keyfile);

      private:
        /// LVM snapshot device name for lvcreate.
        std::string snapshot_device;
        /// LVM snapshot options for lvcreate.
        std::string snapshot_options;
      };

    }
  }
}

#endif /* SBUILD_CHROOT_FACET_LVM_SNAPSHOT_H */

/*
 * Local Variables:
 * mode:C++
 * End:
 */
