/* clientcursor.h */

/**
*    Copyright (C) 2008 10gen Inc.
*
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Cursor -- and its derived classes -- are our internal cursors.

   ClientCursor is a wrapper that represents a cursorid from our database
   application's perspective.
*/

#pragma once

#include "../stdafx.h"
#include "cursor.h"
#include "jsobj.h"
#include "../util/message.h"
#include "storage.h"
#include "dbhelpers.h"
#include "matcher.h"

namespace mongo {

    typedef long long CursorId; /* passed to the client so it can send back on getMore */
    class Cursor; /* internal server cursor base class */
    class ClientCursor;
    typedef map<CursorId, ClientCursor*> CCById;
    typedef multimap<DiskLoc, ClientCursor*> CCByLoc;

    extern BSONObj id_obj;

    class ClientCursor {
        friend class CmdCursorInfo;
        static CCById clientCursorsById;
        static CCByLoc byLoc;
        DiskLoc _lastLoc; // use getter and setter not this.
        unsigned _idleAgeMillis; // how long has the cursor been around, relative to server idle time
        bool _liveForever; // if true, never time out cursor
        static CursorId allocCursorId();
    public:
        ClientCursor() : _idleAgeMillis(0), _liveForever(false), cursorid( allocCursorId() ), pos(0) {
            clientCursorsById.insert( make_pair(cursorid, this) );
        }
        ~ClientCursor();
        const CursorId cursorid;
        string ns;
        auto_ptr<KeyValJSMatcher> matcher;
        auto_ptr<Cursor> c;
        int pos; /* # objects into the cursor so far */
        DiskLoc lastLoc() const {
            return _lastLoc;
        }
        void setLastLoc(DiskLoc);
        auto_ptr< FieldMatcher > filter; // which fields query wants returned
        Message originalMessage; // this is effectively an auto ptr for data the matcher points to

        /* Get rid of cursors for namespaces that begin with nsprefix.
           Used by drop, deleteIndexes, dropDatabase.
        */
        static void invalidate(const char *nsPrefix);

        static bool erase(CursorId id) {
            ClientCursor *cc = find(id);
            if ( cc ) {
                delete cc;
                return true;
            }
            return false;
        }

        static ClientCursor* find(CursorId id, bool warn = true) {
            CCById::iterator it = clientCursorsById.find(id);
            if ( it == clientCursorsById.end() ) {
                if ( warn )
                    OCCASIONALLY out() << "ClientCursor::find(): cursor not found in map " << id << " (ok after a drop)\n";
                return 0;
            }
            return it->second;
        }

        /* call when cursor's location changes so that we can update the
           cursorsbylocation map.  if you are locked and internally iterating, only
           need to call when you are ready to "unlock".
           */
        void updateLocation();

        void cleanupByLocation(DiskLoc loc);
        
        void mayUpgradeStorage() {
/*            if ( !ids_.get() )
                return;
            stringstream ss;
            ss << ns << "." << cursorid;
            ids_->mayUpgradeStorage( ss.str() );*/
        }

        /**
         * @param millis amount of idle passed time since last call
         */
        bool shouldTimeout( unsigned millis ){
            _idleAgeMillis += millis;
            return ! _liveForever && _idleAgeMillis > 600000;
        }
        
        unsigned idleTime(){
            return _idleAgeMillis;
        }

        void liveForever(){
            _liveForever = true;
        }

        static unsigned byLocSize();        // just for diagnostics
        static void idleTimeReport(unsigned millis);

        static void aboutToDeleteBucket(const DiskLoc& b);
        static void aboutToDelete(const DiskLoc& dl);
    };
    
} // namespace mongo
