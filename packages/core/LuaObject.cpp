/*
 * Copyright (c) 2021, University of Washington
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the University of Washington nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY OF WASHINGTON AND CONTRIBUTORS
 * “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY OF WASHINGTON OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "LuaObject.h"
#include "LuaEngine.h"
#include "EventLib.h"
#include "StringLib.h"
#include "OsApi.h"

/******************************************************************************
 * STATIC DATA
 ******************************************************************************/

const char* LuaObject::BASE_OBJECT_TYPE = "LuaObject";
Dictionary<LuaObject*> LuaObject::globalObjects;
Mutex LuaObject::globalMut;

/******************************************************************************
 * PUBLIC METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * getType
 *----------------------------------------------------------------------------*/
const char* LuaObject::getType (void)
{
    if(ObjectType)  return ObjectType;
    else            return "<untyped>";
}

/*----------------------------------------------------------------------------
 * getName
 *----------------------------------------------------------------------------*/
const char* LuaObject::getName (void)
{
    if(ObjectName)  return ObjectName;
    else            return "<unnamed>";
}

/*----------------------------------------------------------------------------
 * getLuaNumParms
 *----------------------------------------------------------------------------*/
int LuaObject::getLuaNumParms (lua_State* L)
{
    return lua_gettop(L);
}

/*----------------------------------------------------------------------------
 * luaGetByName
 *----------------------------------------------------------------------------*/
int LuaObject::luaGetByName(lua_State* L)
{
    bool verbose = true;
    try
    {
        LuaObject* lua_obj = NULL;
        globalMut.lock();
        {
            /* Get Parameters */
            const char* name = getLuaString(L, 1);
            verbose = getLuaBoolean(L, 2, true, true);

            /* Get Self */
            lua_obj = globalObjects.get(name);

            /* Return Lua Object */
            associateMetaTable(L, lua_obj->LuaMetaName, lua_obj->LuaMetaTable);
        }
        globalMut.unlock();
        return createLuaObject(L, lua_obj);
    }
    catch(const RunTimeException& e)
    {
        globalMut.unlock();
        if(verbose) mlog(e.level(), "Failed to get Lua object by name: %s", e.what());
        lua_pushnil(L);
    }

    return 1;
}

/*----------------------------------------------------------------------------
 * getLuaInteger
 *----------------------------------------------------------------------------*/
long LuaObject::getLuaInteger (lua_State* L, int parm, bool optional, long dfltval, bool* provided)
{
    if(provided) *provided = false;

    if(lua_isinteger(L, parm))
    {
        if(provided) *provided = true;
        return lua_tointeger(L, parm);
    }
    else if(optional && ((lua_gettop(L) < parm) || lua_isnil(L, parm)))
    {
        return dfltval;
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "must supply an integer for parameter #%d", parm);
    }
}

/*----------------------------------------------------------------------------
 * getLuaFloat
 *----------------------------------------------------------------------------*/
double LuaObject::getLuaFloat (lua_State* L, int parm, bool optional, double dfltval, bool* provided)
{
    if(provided) *provided = false;

    if(lua_isnumber(L, parm))
    {
        if(provided) *provided = true;
        return lua_tonumber(L, parm);
    }
    else if(optional && ((lua_gettop(L) < parm) || lua_isnil(L, parm)))
    {
        return dfltval;
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "must supply a floating point number for parameter #%d", parm);
    }
}

/*----------------------------------------------------------------------------
 * getLuaBoolean
 *----------------------------------------------------------------------------*/
bool LuaObject::getLuaBoolean (lua_State* L, int parm, bool optional, bool dfltval, bool* provided)
{
    if(provided) *provided = false;

    if(lua_isboolean(L, parm))
    {
        if(provided) *provided = true;
        return lua_toboolean(L, parm);
    }
    else if(optional && ((lua_gettop(L) < parm) || lua_isnil(L, parm)))
    {
        return dfltval;
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "must supply a boolean for parameter #%d", parm);
    }
}

/*----------------------------------------------------------------------------
 * getLuaString
 *----------------------------------------------------------------------------*/
const char* LuaObject::getLuaString (lua_State* L, int parm, bool optional, const char* dfltval, bool* provided)
{
    if(provided) *provided = false;

    if(lua_isstring(L, parm))
    {
        if(provided) *provided = true;
        return lua_tostring(L, parm);
    }
    else if(optional && ((lua_gettop(L) < parm) || lua_isnil(L, parm)))
    {
        return dfltval;
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "must supply a string for parameter #%d", parm);
    }
}

/*----------------------------------------------------------------------------
 * returnLuaStatus
 *----------------------------------------------------------------------------*/
int LuaObject::returnLuaStatus (lua_State* L, bool status, int num_obj_to_return)
{
    if(!status) lua_pushnil(L);
    else
    {
        if( num_obj_to_return == 1 )
        {
            int stack_cnt = lua_gettop(L);

            /* Self object must be on stack */
            assert(stack_cnt != 0);

            lua_pop(L, stack_cnt - 1);
            /* Return self as status, allow to chain calls */
        }
        else lua_pushboolean(L, true);
    }

    return num_obj_to_return;
}

/*----------------------------------------------------------------------------
 * getLuaObjectByName
 *----------------------------------------------------------------------------*/
LuaObject* LuaObject::getLuaObjectByName (const char* name, const char* object_type)
{
    LuaObject* lua_obj = NULL;
    globalMut.lock();
    {
        try
        {
            LuaObject* obj = globalObjects.get(name);
            if(StringLib::match(obj->getType(), object_type))
            {
                lua_obj = obj;
                lua_obj->referenceCount++;
            }
        }
        catch(const RunTimeException& e)
        {
        }
    }
    globalMut.unlock();

    return lua_obj;
}

/*----------------------------------------------------------------------------
 * releaseLuaObject
 *----------------------------------------------------------------------------*/
bool LuaObject::releaseLuaObject (void)
{
    bool is_delete_pending = false;

    /* Decrement Reference Count */
    referenceCount--;
    if(referenceCount == 0)
    {
        mlog(DEBUG, "Delete on release for object %s/%s", getType(), getName());
        is_delete_pending = true;
    }
    else if(referenceCount < 0)
    {
        mlog(CRITICAL, "Unmatched object release %s of type %s detected", getName(), getType());
    }

    /* Delete THIS Object */
    if(is_delete_pending)
    {
        delete this;
    }

    return is_delete_pending;
}

/******************************************************************************
 * PROTECTED METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
LuaObject::LuaObject (lua_State* L, const char* object_type, const char* meta_name, const struct luaL_Reg meta_table[]):
    ObjectType(object_type),
    ObjectName(NULL),
    LuaMetaName(meta_name),
    LuaMetaTable(meta_table),
    LuaState(L)
{
    uint32_t engine_trace_id = ORIGIN;

    referenceCount = 0;
    objComplete = false;

    if(LuaState)
    {
        /* Trace from Lua Engine */
        lua_getglobal(LuaState, LuaEngine::LUA_TRACEID);
        engine_trace_id = lua_tonumber(LuaState, -1);

        /* Associate Meta Table */
        associateMetaTable(LuaState, meta_name, meta_table);
        mlog(DEBUG, "Created object of type %s/%s", getType(), LuaMetaName);
    }

    /* Start Trace */
    traceId = start_trace(DEBUG, engine_trace_id, "lua_object", "{\"object_type\":\"%s\", \"meta_name\":\"%s\"}", object_type, meta_name);
}

/*----------------------------------------------------------------------------
 * Destructor
 *----------------------------------------------------------------------------*/
LuaObject::~LuaObject (void)
{
    stop_trace(DEBUG, traceId);
    mlog(DEBUG, "Deleting %s/%s", getType(), getName());

    /* Remove Name from Global Objects */
    globalMut.lock();
    {
        /* Remove Previous Name (if it exists) */
        if(ObjectName)
        {
            globalObjects.remove(ObjectName);
            delete [] ObjectName;
        }
    }
    globalMut.unlock();
}

/*----------------------------------------------------------------------------
 * luaDelete
 *----------------------------------------------------------------------------*/
int LuaObject::luaDelete (lua_State* L)
{
    try
    {
        luaUserData_t* user_data = (luaUserData_t*)lua_touserdata(L, 1);
        if(user_data)
        {
            LuaObject* lua_obj = user_data->luaObj;
            if(lua_obj)
            {
                int count = lua_obj->referenceCount--;
                mlog(DEBUG, "Garbage collecting object %s/%s <%d>", lua_obj->getType(), lua_obj->getName(), count);

                if(lua_obj->referenceCount == 0)
                {
                    /* Delete Object */
                    delete lua_obj;
                    user_data->luaObj = NULL;
                }
                else
                {
                    mlog(DEBUG, "Delaying delete on referenced object %s/%s <%d>", lua_obj->getType(), lua_obj->getName(), count);
                }
            }
            else
            {
                /* This will occurr, for instance, when a device is closed
                 * explicitly, and then also deleted when the lua variable
                 * goes out of scope and is garbage collected */
                mlog(DEBUG, "Vacuous delete of lua object that has already been deleted");
            }
        }
        else
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "unable to retrieve user data");
        }
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error deleting object: %s", e.what());
    }

    return 0;
}


/*----------------------------------------------------------------------------
 * luaName
 *----------------------------------------------------------------------------*/
int LuaObject::luaName(lua_State* L)
{
    bool status = false;

    try
    {
        /* Get Self */
        LuaObject* lua_obj = getLuaSelf(L, 1);

        /* Get Name */
        const char* name = getLuaString(L, 2);

        /* Add Name to Global Objects */
        globalMut.lock();
        {
            /* Check if Name Already Exists */
            if(!lua_obj->ObjectName)
            {
                /* Register Name */
                if(globalObjects.add(name, lua_obj, true))
                {
                    /* Associate Name */
                    lua_obj->ObjectName = StringLib::duplicate(name);
                    mlog(DEBUG, "Associating %s with object of type %s", lua_obj->getName(), lua_obj->getType());
                    status = true;
                }
                else
                {
                    mlog(WARNING, "Name conflict on %s for type %s", lua_obj->getName(), lua_obj->getType());
                    status = true; // do not treat as error
                }
            }
            else
            {
                mlog(WARNING, "Object already named %s, cannot overwrite with name %s", lua_obj->getName(), name);
                status = true; // do not treat as error
            }
        }
        globalMut.unlock();

        /* Check for Errors */
        if(!status) throw RunTimeException(CRITICAL, RTE_ERROR, "Unable to register name: %s", name);

        /* Pop name */
        lua_pop(L, 1);

        /* Stack holds Self */
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error associating object: %s", e.what());
        lua_pushnil(L);
    }

    return 1;
}

/*----------------------------------------------------------------------------
 * luaWaitOn - :waiton([<timeout is milliseconds>])
 *----------------------------------------------------------------------------*/
int LuaObject::luaWaitOn(lua_State* L)
{
    bool status = false;

    try
    {
        /* Get Self */
        LuaObject* lua_obj = getLuaSelf(L, 1);

        /* Get Parameters */
        int timeout = getLuaInteger(L, 2, true, IO_PEND);

        /* Wait On Signal */
        lua_obj->objSignal.lock();
        {
            if(!lua_obj->objComplete)
            {
                lua_obj->objSignal.wait(SIGNAL_COMPLETE, timeout);
            }
            status = lua_obj->objComplete;
        }
        lua_obj->objSignal.unlock();
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error locking object: %s", e.what());
    }

    /* Return Completion Status */
    return returnLuaStatus(L, status);
}

/*----------------------------------------------------------------------------
 * signalComplete
 *----------------------------------------------------------------------------*/
void LuaObject::signalComplete (void)
{
    objSignal.lock();
    {
        if(!objComplete)
        {
            objSignal.signal(SIGNAL_COMPLETE);
        }
        objComplete = true;
    }
    objSignal.unlock();
}

/*----------------------------------------------------------------------------
 * associateMetaTable
 *----------------------------------------------------------------------------*/
void LuaObject::associateMetaTable (lua_State* L, const char* meta_name, const struct luaL_Reg meta_table[])
{
    if(luaL_newmetatable(L, meta_name))
    {
        /* Add Child Class Functions */
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
        luaL_setfuncs(L, meta_table, 0);

        /* Add Base Class Functions */
        LuaEngine::setAttrFunc(L, "name", luaName);
        LuaEngine::setAttrFunc(L, "getbyname", luaGetByName);
        LuaEngine::setAttrFunc(L, "waiton", luaWaitOn);
        LuaEngine::setAttrFunc(L, "destroy", luaDelete);
        LuaEngine::setAttrFunc(L, "__gc", luaDelete);
    }
}

/*----------------------------------------------------------------------------
 * createLuaObject
 *
 *  Note: if object is an alias, all calls into it from Lua must be thread safe
 *----------------------------------------------------------------------------*/
int LuaObject::createLuaObject (lua_State* L, LuaObject* lua_obj)
{
    /* Create Lua User Data Object */
    luaUserData_t* user_data = (luaUserData_t*)lua_newuserdata(L, sizeof(luaUserData_t));
    if(!user_data)
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "failed to allocate new user data");
    }

    /* Bump Reference Count */
    lua_obj->referenceCount++;

    /* Return User Data to Lua */
    user_data->luaObj = lua_obj;
    luaL_getmetatable(L, lua_obj->LuaMetaName);
    lua_setmetatable(L, -2);
    return 1;
}

/*----------------------------------------------------------------------------
 * getLuaObject
 *----------------------------------------------------------------------------*/
LuaObject* LuaObject::getLuaObject (lua_State* L, int parm, const char* object_type, bool optional, LuaObject* dfltval)
{
    LuaObject* lua_obj = NULL;
    luaUserData_t* user_data = (luaUserData_t*)lua_touserdata(L, parm);
    if(user_data)
    {
        if(StringLib::match(object_type, user_data->luaObj->ObjectType))
        {
            lua_obj = user_data->luaObj;
            user_data->luaObj->referenceCount++;
        }
        else
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "%s object returned incorrect type <%s.%s>", object_type, user_data->luaObj->ObjectType, user_data->luaObj->LuaMetaName);
        }
    }
    else if(optional && ((lua_gettop(L) < parm) || lua_isnil(L, parm)))
    {
        return dfltval;
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "calling object method from something not an object");
    }

    return lua_obj;
}

/*----------------------------------------------------------------------------
 * getLuaSelf
 *----------------------------------------------------------------------------*/
LuaObject* LuaObject::getLuaSelf (lua_State* L, int parm)
{
    luaUserData_t* user_data = (luaUserData_t*)lua_touserdata(L, parm);
    if(user_data)
    {
        if(user_data->luaObj)
        {
            if(luaL_testudata(L, parm, user_data->luaObj->LuaMetaName))
            {
                return user_data->luaObj;
            }
            else
            {
                throw RunTimeException(CRITICAL, RTE_ERROR, "object method called from inconsistent type <%s>", user_data->luaObj->LuaMetaName);
            }
        }
        else
        {
            throw RunTimeException(CRITICAL, RTE_ERROR, "object method called on emtpy object");
        }
    }
    else
    {
        throw RunTimeException(CRITICAL, RTE_ERROR, "calling object method from something not an object");
    }
}

/*----------------------------------------------------------------------------
 * referenceLuaObject
 *----------------------------------------------------------------------------*/
void LuaObject::referenceLuaObject (LuaObject* lua_obj)
{
    lua_obj->globalMut.lock();
    {
        lua_obj->referenceCount++;
    }
    lua_obj->globalMut.unlock();
}
