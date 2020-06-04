/*
 * Licensed to the University of Washington under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The University of Washington
 * licenses this file to you under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/******************************************************************************
 *INCLUDES
 ******************************************************************************/

#include "core.h"
#include "ccsds.h"
#include "legacy.h"
#include "sigview.h"

/******************************************************************************
 * EXPORTED FUNCTIONS
 ******************************************************************************/

extern "C" {
void initsigview (void)
{
    extern CommandProcessor* cmdProc;

    /* Register SigView Handlers */
    cmdProc->registerHandler("ATLAS_FILE_WRITER",        AtlasFileWriter::createObject,             -3,  "<format: SCI_PKT, SCI_CH, SCI_TX, HISTO, CCSDS_STAT, CCSDS_INFO, META, CHANNEL, ACVPT, TIMEDIAG, TIMESTAT> <file prefix including path> <input stream>");
    cmdProc->registerHandler("ADAS_READER",              AdasSocketReader::createObject,             3,  "<ip address> <port> <output stream>");
    cmdProc->registerHandler("ITOS_RECORD_PARSER",       ItosRecordParser::createObject,             0,  "", true);
    cmdProc->registerHandler("DATASRV_READER",           DatasrvSocketReader::createObject,         -1,  "<ip address> <port> <output stream> <start time> <stop time> <request arch string> <apid list>");
    cmdProc->registerHandler("TIME_TAG_PROCESSOR",       TimeTagProcessorModule::createObject,       3,  "<histogram stream> <Tx time stream>  <pce: 1,2,3>", true);
    cmdProc->registerHandler("ALTIMETRY_PROCESSOR",      AltimetryProcessorModule::createObject,     3,  "<histogram type: SAL, WAL, SAM, WAM, ATM> <histogram stream> <pce: 1,2,3>", true);
    cmdProc->registerHandler("MAJOR_FRAME_PROCESSOR",    MajorFrameProcessorModule::createObject,    0,  "", true);
    cmdProc->registerHandler("TIME_PROCESSOR",           TimeProcessorModule::createObject,          0,  "", true);
    cmdProc->registerHandler("LASER_PROCESSOR",          LaserProcessorModule::createObject,         0,  "", true);
    cmdProc->registerHandler("BCE_PROCESSOR",            BceProcessorModule::createObject,           1,  "<histogram output stream>", true);
    cmdProc->registerHandler("CMD_ECHO_PROCESSOR",       CmdEchoProcessorModule::createObject,      -1,  "<echo stream> <itos record parser: NULL if not specified> [<pce: 1,2,3>]", true);
    cmdProc->registerHandler("DIAG_LOG_PROCESSOR",       DiagLogProcessorModule::createObject,      -1,  "<diagnostic log stream> [<pce: 1,2,3>]", true);
    cmdProc->registerHandler("REPORT_STATISTIC",         ReportProcessorStatistic::createObject,     6,  "<pce 1 time tag processor> <pce 2 time tag processor> <pce 3 time tag processor> <time processor> <bce processor> <laser processor>");
    cmdProc->registerHandler("HSTVS_SIMULATOR",          HstvsSimulator::createObject,               1,  "<histogram stream>");
    cmdProc->registerHandler("BLINK_PROCESSOR",          BlinkProcessorModule::createObject,         1,  "<time processor name>", true);
    cmdProc->registerHandler("TX_TIME_PROCESSOR",        TxTimeProcessor::createObject,              2,  "<Tx time stream> <pce: 1,2,3>");

    /* Indicate Presence of Package */
    LuaEngine::indicate("sigview", BINID);

    /* Display Status */
    printf("sigview plugin initialized (%s)\n", BINID);
}
}
