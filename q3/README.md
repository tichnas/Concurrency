# Musical Mayhem

## Input

1. First line of input contains _k_ (no. of performers), _a_ (no. of acoustic stages), _e_ (no. of electric stages), _c_ (no. of coordinators), _t1_ (min. performance time), _t2_ (max. performance time), _t_ (max. waiting time)
2. Next _k_ lines contain each performer in the form: _name_ _instrument character_ _arrival time_

```
5 1 1 4 2 10 5
Tanvi p 0
Sudh b 1
Manas g 0
Sriya v 1
Pahun s 1
```

## Function & Structure Descriptions

- `Performer`

  - Structure to store performer details

- `Stage`

  - Structure to store stage details

- `PerformerCarrier`

  - Structure to carry a pointer to a performer with the type of stages it want

- `updateTime`

  - Update `timeStr` variable with the seconds passed

- `random`

  - Takes min and max as parameter
  - Returns a random number

- `maxWait`

  - Takes a pointer to a Performer struct as parameter
  - Sleeps for max. waiting time
  - Checks if the performer is still waiting and make him leave if required

- `soloPerform`

  - Takes a pointer to a Performer struct as parameter
  - Sleeps for random performance duration
  - After that, checks if the singer was also there and sleep for 2 more seconds if required
  - Signal the singer (if required)
  - Clear the stage

- `jointPerform`

  - Takes a pointer to a Performer struct as parameter
  - Waits for the musician to signal and then exits

- `getTShirt`

  - Takes a pointer to a Performer struct as parameter
  - Wait for a free coordinator
  - Sleeps for 2 seconds
  - Release the coordinator

- `handlePerformer`

  - Takes a pointer to a PerformerCarrier struct as parameter
  - Waits for required case in case of musician and trywait for both types of stages in case of singer
  - Checks if the counterpart if finished (musicians who can perform on both the stages are broken into two parts)
  - Loops over stages to find a compatible stage
  - If found, perform on the stage using either `soloPerform` or `jointPerform`
  - After finishing the performance, go for the T-Shirt

- `performerJob`

  - Takes a pointer to a Performer struct as parameter
  - Sleeps for the arrival time
  - Depending on type of performer, create the PerformerCarrier struct
  - Create thread to run `handlePerformer` with the struct as argument

- `main`
  - Starter function
  - Takes input, creates thread and starts the simulation
