# 1. Data Broker Query Syntax

- [1. Data Broker Query Syntax](#1-data-broker-query-syntax)
  - [1.1. Intro](#11-intro)
  - [1.2. Subscribe to single datapoint with a single condition](#12-subscribe-to-single-datapoint-with-a-single-condition)
    - [1.2.1. Time / conceptual row view](#121-time--conceptual-row-view)
    - [1.2.2. Responses](#122-responses)
  - [1.3. Subscribing to multiple data points / signals](#13-subscribing-to-multiple-data-points--signals)
    - [1.3.1. Responses](#131-responses)
  - [1.4. Returning multiple signals and a condition](#14-returning-multiple-signals-and-a-condition)
    - [1.4.1. Responses](#141-responses)
- [2. Future](#2-future)
  - [2.1. Introduce LAG() function,](#21-introduce-lag-function)
    - [2.1.1. Time / conceptual row view](#211-time--conceptual-row-view)
    - [2.1.2. Responses](#212-responses)
  - [2.2. React on condition changes](#22-react-on-condition-changes)
    - [2.2.1. Time / conceptual row view](#221-time--conceptual-row-view)
    - [2.2.2. Responses](#222-responses)

## 1.1. Intro
This document describes the behaviour of the current implementation.

The query syntax is a subset of SQL and is used to select which data
points (or conditional expressions) are to be returned if the WHERE clause
evaluates to true.


## 1.2. Subscribe to single datapoint with a single condition

Subscribe to:
```sql
SELECT
    Vehicle.Seats.Row1.Position
WHERE
    Vehicle.Datapoint2 > 50
```

Get a response every time `Vehicle.Seats.Row1.Position` or
`Vehicle.Datapoint2` is updated (and the condition is true)

### 1.2.1. Time / conceptual row view

Each time any datapoint changes a new "conceptual row" is created.
It contains the values of all datapoints at that point in time.


| #| ...Datapoint1 | ...Datapoint2 | ...Row1.Position | Response |   |
|-:|:-------------:|:-------------:|:----------------:|----------|:--|
| 1|  15           | 30            | 250  |    | Query posted, nothing is returned since (`Vehicle.Datapoint2 > 50`) isn't true |
| 2|  20*          | 30            | 250  |    | |
| 3|  20           | 30            | 240* |    | |
| 4|  20           | 40*           | 240  |    | |
| 5|  20           | 40            | 230* |    | |
| 6|  23           | 60*           | 230  | #1 | (`Vehicle.Datapoint2 > 50`) turns true |
| 7|  23           | 61*           | 230  | #2 | `Vehicle.Datapoint2` changed |
| 8|  21*          | 61            | 230  |    | |
| 9|  21           | 61            | 220* | #3 | `Vehicle.Seats.Row1.Position` changed |
|10|  21           | 62*           | 220  | #4 | `Vehicle.Datapoint2` changed |
|11|  21           | 62            | 210* | #5 | `Vehicle.Datapoint2` changed |

### 1.2.2. Responses
|  | Vehicle.Seats.Row1.Position |
|-:|:---------------------------:|
| 1|            230              |
| 2|            230              |
| 3|            220              |
| 4|            220              |
| 5|            210              |

## 1.3. Subscribing to multiple data points / signals

Using a single condition and in this examples using `AS` to name
the response fields.

Subscribe to:
```sql
SELECT
    Vehicle.Seats.Row1.Position AS pos1,
    Vehicle.Seats.Row2.Position AS pos2
WHERE
    Vehicle.Datapoint2 > 50
```

A message is received every time `Vehicle.Seats.Row1.Position`, `Vehicle.Seats.Row2.Position` or `Vehicle.Datapoint2` is updated (and the condition is true)


| #| ...Datapoint2 | ...Row1.Position | ...Row2.Position |Response |   |
|-:|:-------------:|:----------------:|:----------------:|---------|:--|
| 1| 30            | 250              | 150              |    | Query posted, nothing is returned since (`Vehicle.Datapoint2 > 50`) isn't true |
| 2| 30            | 250              | 150              |    | |
| 3| 30            | 240*             | 150              |    | |
| 4| 30            | 240              | 160*             |    | |
| 5| 40*           | 240              | 160              |    | |
| 6| 60*           | 240              | 160              | #1 | (`Vehicle.Datapoint2 > 50`) turns true |
| 7| 61*           | 240              | 160              | #2 | `Vehicle.Datapoint2` changed |
| 8| 61            | 240              | 160              |    | (some unrelated signal changed) |
| 9| 61            | 240              | 180*             | #3 | `Vehicle.Seats.Row2.Position` changed |
|10| 62*           | 240              | 180              | #4 | `Vehicle.Datapoint2` changed |
|11| 62            | 230*             | 180              | #5 | `Vehicle.Seats.Row2.Position` changed |

### 1.3.1. Responses
|  | pos1 | pos2 |
|-:|:----:|:----:|
| 1| 240  | 160  |
| 2| 240  | 160  |
| 3| 240  | 180  |
| 4| 240  | 180  |
| 5| 230  | 180  |

## 1.4. Returning multiple signals and a condition

Conditions are possible to select (and name) as well.

Subscribe to:
```sql
SELECT
    Vehicle.Seats.Row1.Position AS pos1,
    Vehicle.Seats.Row2.Position AS pos2,
    (Vehicle.Speed = 0) AS cond1
WHERE
    Vehicle.Datapoint2 BETWEEN 50 AND 150
    AND
    Vehicle.Seats.Row1.Position > 100
```

A message is received every time `Vehicle.Seats.Row1.Position`, `Vehicle.Seats.Row2.Position` or
`Vehicle.Datapoint2` is updated (and the condition is true)


| #| ...Datapoint2 | pos1 | pos2 | ..Speed | Response  |    |
|-:|:-------------:|:----:|:----:|---------|:----------|:---|
| 1| 30            | 250   | 150  | 30       |    | Query posted, nothing is returned since (`Vehicle.Datapoint2 > 50`) isn't true |
| 2| 30            | 250   | 150  | 30       |    | |
| 3| 30            | 240*  | 150  | 30       |    | |
| 4| 30            | 240   | 160* | 30       |    | |
| 5| 40*           | 240   | 160  | 30       |    | |
| 6| 60*           | 240   | 160  | 30       | #1 | (`Vehicle.Datapoint2 > 50`) turns true |
| 7| 61*           | 240   | 160  | 30       | #2 | `Vehicle.Datapoint2` changed |
| 8| 61            | 240   | 160  | 20       |    | (some unrelated signal changed) |
| 9| 61            | 240   | 180* | 10       | #3 | `Vehicle.Seats.Row2.Position` changed |
|10| 61            | 240   | 180  | 0*       | #4 | `Vehicle.Speed` changed dependent cond1 |
|11| 61            | 230*  | 180  | 0        | #5 | `Vehicle.Seats.Row2.Position` changed |


### 1.4.1. Responses
|  | pos1 |pos2 | cond1   |
|-:|:----:|:---:|:-------:|
| 1| 240  | 160 | `false` |
| 2| 240  | 160 | `false` |
| 3| 240  | 180 | `false` |
| 4| 240  | 180 | `true`  |
| 5| 230  | 180 | `true`  |

# 2. Future

What follows isn't implemented or fully thought through yet.

```
 _____ _   _ _____ _   _ ____  _____
|  ___| | | |_   _| | | |  _ \| ____|
| |_  | | | | | | | | | | |_) |  _|
|  _| | |_| | | | | |_| |  _ <| |___
|_|    \___/  |_|  \___/|_| \_\_____|

   /  BRAINSTORMING FOLLOWS
```

## 2.1. Introduce LAG() function,

Implement and use the `LAG()` function to be able to match current values with values from the past.

This can be used to only receive updates if `Vehicle.Seats.Row1.Position` actually
changes (and the original condition is met).

Subscribe to:
```sql
SELECT
    Vehicle.Seats.Row1.Position
WHERE
    Vehicle.Seats.Row1.Position <> LAG(Vehicle.Seats.Row1.Position)
    AND
    Vehicle.Datapoint2 > 50
```

Get a response every time `Vehicle.Seats.Row1.Position` is changed (and the condition is true).

### 2.1.1. Time / conceptual row view

| #| ...Datapoint1 | ...Datapoint2 | ...Row1.Position | Response |   |
|-:|:-------------:|:-------------:|:----------------:|:--------:|:--|
| 1| 15*           | 30            | 250              |   | Condition not fulfilled, nothing initially returned|
| 2| 20            | 30*           | 250              |   ||
| 3| 20            | 30            | 240*             |   ||
| 4| 20            | 40*           | 240              |   ||
| 5| 20            | 40            | 230*             |   ||
| 6| 23            | 60*           | 230              | #1|`Vehicle.Datapoint2 > 50` turns true |
| 7| 23            | 61*           | 230              |   ||
| 8| 21*           | 61            | 230              |   ||
| 9| 21            | 61            | 220*             | #2|`Vehicle.Seats.Row1.Position` changed|
|10| 21            | 62*           | 220              |   ||
|11| 21            | 62            | 210*             | #3|`Vehicle.Seats.Row1.Position` changed|

### 2.1.2. Responses
|  | Vehicle.Seats.Row1.Position |
|-:|:---------------------------:|
| 1|             230             |
| 2|             220             |
| 3|             210             |

## 2.2. React on condition changes

 Example with two additional conditions, using the
 the `LAG()` function to only send updates if `Vehicle.Seats.Row1.Position`
 changes (and the condition is met).
 The result includes the evaluation of the condition, named `cond` in this case,
 in the projection (the SELECT statement).

 This gives the client:
 1. An initial response even when the condition isn't
    met, along with an easy way to tell this is the case
    (by checking if cond == true)
 2. A notification when the condition is no longer met.

```sql
SELECT
    Vehicle.Seats.Row1.Position AS pos,  -- current position
    (Vehicle.Datapoint2 > 50) AS cond    -- cond
WHERE
    ( -- send update if condition is met & position is updated
        Vehicle.Seats.Row1.Position <> LAG(pos)
        AND
        (Vehicle.Datapoint2 > 50)
    )
    OR
    ( -- Also send update if condition changes
        (Vehicle.Datapoint2 > 50) <> LAG((Vehicle.Datapoint2 > 50))
    )
```

### 2.2.1. Time / conceptual row view

| #| ...Datapoint1 | ...Datapoint2 | ...Row1.Position | Response |   |
|-:|:-------------:|:-------------:|:----------------:|:--------:|:--|
|1 |  *15          |        30     |             250  | #1 | |
|2 |   20          |       *30     |             250  |    | |
|3 |   20          |        30     |            *240  |    | |
|4 |   20          |       *40     |             240  |    | |
|5 |   20          |        40     |            *230  |    | |
|6 |   23          |       *60     |             230  | #2 | |
|7 |   23          |       *61     |             230  |    | |
|8 |  *21          |        61     |             230  |    | |
|9 |   21          |        61     |            *220  | #3 | |
|10|   21          |       *62     |             220  |    | |
|11|   21          |        62     |            *210  | #4 | |
|12|   21          |       *55     |             210  |    | |
|13|   21          |       *49     |             210  | #5 | |


### 2.2.2. Responses
|  | pos |  cond |
|-:|:---:|:-----:|
| 1| 250 |`false`|
| 2| 230 | `true`|
| 3| 220 | `true`|
| 4| 220 | `true`|
| 5| 210 |`false`|
