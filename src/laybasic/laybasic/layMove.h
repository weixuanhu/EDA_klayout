
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/



#ifndef HDR_layMove
#define HDR_layMove

#include "dbManager.h"
#include "layViewObject.h"

#include <QTimer>
#include <QObject>

#include <memory>

namespace lay {

class Editables;
class LayoutView;

class MoveService
  : public QObject,
    public lay::ViewService
{
Q_OBJECT 

public: 
  MoveService (lay::LayoutView *view);
  ~MoveService ();

  virtual bool configure (const std::string &name, const std::string &value);
  bool begin_move (db::Transaction *transaction = 0);

private:
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio);
  virtual bool wheel_event (int delta, bool horizonal, const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool key_event (unsigned int key, unsigned int buttons);
  virtual void drag_cancel ();
  virtual void deactivated ();

  bool handle_dragging (const db::DPoint &p, unsigned int buttons, db::Transaction *transaction);

  bool m_dragging;
  lay::Editables *mp_editables;
  lay::LayoutView *mp_view;
  double m_global_grid;
  db::DPoint m_shift;
  std::auto_ptr<db::Transaction> mp_transaction;
};

}

#endif

