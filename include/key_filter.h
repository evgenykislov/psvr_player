/*
 * Created by Evgeny Kislov <dev@evgenykislov.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef KEY_FILTER_17092022_H
#define KEY_FILTER_17092022_H

#include <QObject>
#include <QEvent>


class KeyFilter: public QObject {
  Q_OBJECT

 public:

  virtual bool eventFilter(QObject*, QEvent* event) override;

 signals:
   void FullStop();
   void ResetView();
   void FullScreen();
   void CompensateView();

   void Up();
   void Down();
   void Left();
   void Right();
   void Select();
};



#endif // KEY_FILTER_H
