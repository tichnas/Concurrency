# Back to College

## Input

1. First line of input contains _n_ (no. of companies), _m_ (no. of zones) and _o_ (no. of students)
2. Next _n_ numbers are success probabilities of vaccine for each company

```
2 10 50
0.3 0.7
```

## Function & Structure Descriptions

- `company`

  - Structure to store company details

- `zone`

  - Structure to store zone details

- `student`

  - Structure to store student details

- `updateTime`

  - Update `timeStr` variable with the seconds passed

- `min`

  - Takes 3 numbers as parameter
  - Returns the minimum of all three.

- `studentJob`

  - Takes pointer to a student struct as parameter
  - Sleeps for the arrival time
  - Waits till returns from the simulation

- `vaccinateSuccess`

  - Takes student no. as parameter
  - Removes student from simulation

- `vaccinateFailure`

  - Takes student no. as parameter
  - If vaccinations given < 3, then make the student wait for a slot again, otherwise, remove the student from simulation

- `vaccinate`

  - Takes student no. as parameter
  - According to the success probability and bad luck, call `vaccinateSuccess` or `vaccinateFailure`

- `zoneJob`

  - Takes pointer to a zone struct as parameter
  - Sleeps for the vaccine delivery time
  - Zone thread is created everytime the vaccine is delivered and destroyed after no vaccines are left
  - Create slots and loop over students to fill them
  - After filling the slots, start vaccination

- `supply`

  - Takes company no. as parameter
  - Loops over zones to find one waiting for vaccines
  - Start delivering vaccines to zones

- `companyJob`

  - Starts preparing vaccines everytime all previous produced vaccines are used
  - Sleeps for preparation time
  - Supplies vaccines to zones using `supply`
  - Waits till all produced vaccines are not used

- `main`
  - Starter function
  - Takes input, creates thread and starts the simulation

## Assumptions

1. Delivering vaccines take some time
2. Vaccination of students take some time
3. Antibody test is instant
4. Since, the given scenario is a college, we already know the number of students who will come in total. Zones can prepare slots for students who are still not arrived
5. All the zones have a common database to store no. of slots produced. Zone will limit its no. of slots such that total no. of all the slots are not more than waiting students
