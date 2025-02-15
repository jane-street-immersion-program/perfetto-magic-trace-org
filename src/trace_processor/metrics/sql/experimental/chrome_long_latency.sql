--
-- Copyright 2022 The Android Open Source Project
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--     https://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.

-- Find all long EventLatency slices > 100ms and also get the
-- type of the event stored as 'debug.event' argument.
-- In order to group all events
-- Note that a long latency event is represented by the the ending time
-- of an EventLatency slice, i.e. the timestamp of the frame presentation
-- that reflects the event.
DROP VIEW IF EXISTS long_eventlatency_slice;
CREATE VIEW long_eventlatency_slice AS
SELECT
  ts + dur AS ts,
  dur,
  id,
  track_id,
  EXTRACT_ARG(arg_set_id, 'debug.event') AS event_type
FROM slice WHERE name = 'EventLatency' AND dur > 100000000;

-- Find the upid of the proccesses where the long latency occur.
DROP VIEW IF EXISTS long_latency_with_upid;
CREATE VIEW long_latency_with_upid AS
SELECT
  long_eventlatency_slice.ts,
  long_eventlatency_slice.event_type,
  process_track.upid
FROM long_eventlatency_slice
INNER JOIN process_track
ON long_eventlatency_slice.track_id = process_track.id;

-- Find the name and pid of the processes.
-- Long latency events with the same timestamp and from the same process
-- are considered one single long latency occurrence.
DROP VIEW IF EXISTS long_latency_with_process_info;
CREATE VIEW long_latency_with_process_info AS
SELECT
  long_latency_with_upid.ts,
  long_latency_with_upid.event_type,
  process.name AS process_name,
  process.pid AS process_id
FROM long_latency_with_upid
INNER JOIN process
ON long_latency_with_upid.upid = process.upid
GROUP BY ts, process.pid;

-- Create the derived event track for long latency.
-- All tracks generated from chrome_long_latency_event are
-- placed under a track group named 'Long Latency', whose summary
-- track is the first track ('All Processes') in chrome_long_latency_event.
-- Note that the 'All Processes' track is generated only when there are more
-- than one source of long latency events.
DROP VIEW IF EXISTS chrome_long_latency_event;
CREATE VIEW chrome_long_latency_event AS
SELECT
  'slice' AS track_type,
  'All Processes' AS track_name,
  ts,
  0 AS dur,
  event_type AS slice_name,
  'Long Latency' AS group_name
FROM long_latency_with_process_info
WHERE (SELECT COUNT(DISTINCT process_id)
       FROM long_latency_with_process_info) > 1
GROUP BY ts
UNION ALL
SELECT
  'slice' AS track_type,
  process_name || ' ' || process_id AS track_name,
  ts,
  0 AS dur,
  event_type AS slice_name,
  'Long Latency' AS group_name
FROM long_latency_with_process_info
GROUP BY ts;

-- Create the long latency metric output.
DROP VIEW IF EXISTS chrome_long_latency_output;
CREATE VIEW chrome_long_latency_output AS
SELECT ChromeLongLatency(
  'long_latency', (
    SELECT RepeatedField(
      ChromeLongLatency_LongLatency(
        'ts', ts,
        'event_type', event_type,
        'process_name', process_name,
        'pid', process_id
      )
    )
    FROM long_latency_with_process_info
    ORDER BY ts
  )
);