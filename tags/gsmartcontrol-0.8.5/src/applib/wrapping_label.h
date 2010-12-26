/**************************************************************************
 Copyright:
      (C) 2008 - 2009  Alexander Shaduri <ashaduri 'at' gmail.com>
 License: See LICENSE_gsmartcontrol.txt
***************************************************************************/

#ifndef WRAPPING_LABEL_H
#define WRAPPING_LABEL_H

#include <gtkmm/label.h>

#include "app_gtkmm_features.h"  // APP_GTKMM_CONNECT_VIRTUAL


// An auto-wrapping label. The GtkLabel widget has a bug
// (or "design decision", as gtk devs say), when the widget can't
// control its wrapping width according to parent container's size.
// This should fix that.

class WrappingLabel : public Gtk::Label {
	public:

		WrappingLabel(const Glib::ustring& label, Gtk::AlignmentEnum xalign,
				Gtk::AlignmentEnum yalign = Gtk::ALIGN_CENTER, bool mnemonic = false)
				: Gtk::Label(label, xalign, yalign, mnemonic)
		{
			this->construct();
		}

		WrappingLabel(const Glib::ustring& label, float xalign, float yalign, bool mnemonic = false)
				: Gtk::Label(label, xalign, yalign, mnemonic)
		{
			this->construct();
		}

		WrappingLabel (const Glib::ustring& label, bool mnemonic=false)
				: Gtk::Label(label, mnemonic)
		{
			this->construct();
		}

		WrappingLabel()
		{
			this->construct();
		}


		// override parent's similar functions

		void set_text(const Glib::ustring& label)
		{
			Label::set_text(label);
			set_width(width_);
		}

		void set_markup(const Glib::ustring& label)
		{
			Label::set_markup(label);
			set_width(width_);
		}

		void set_label(const Glib::ustring& label)
		{
			Label::set_label(label);
			set_width(width_);
		}


	protected:

		// Override parent's default handlers

		void on_size_allocate_before(Gtk::Allocation& alloc)
		{
			Gtk::Label::on_size_allocate(alloc);
			set_width(alloc.get_width());
		}

		void on_size_request_before(Gtk::Requisition* req)
		{
			int w = 0, h = 0;
			get_layout()->get_pixel_size(w, h);
			req->width  = -1;
			req->height = h;
		}


	private:

		void construct()
		{
			width_ = 0;

			// set_line_wrap_mode is from 2.10, so use this instead.
			get_layout()->set_wrap(Pango::WRAP_WORD_CHAR);

			typedef WrappingLabel self_type;
			APP_GTKMM_CONNECT_VIRTUAL(size_allocate);
			APP_GTKMM_CONNECT_VIRTUAL(size_request);
		}


		void set_width(int width)
		{
			if (width == 0)
				return;
			get_layout()->set_width(width * Pango::SCALE);
			if (width_ != width) {
				width_ = width;
				queue_resize();
			}
		}


		int width_;  // wrapping width

};





#endif
