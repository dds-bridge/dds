# dtest Program

The `dtest` program is a test harness for the Double Dummy Solver (DDS) library. It is designed to verify the correctness of the solver by running it against a series of predefined test cases.

## Test Cases

The test cases are located in the `hands` directory. Each `.txt` file in this directory represents one or more bridge deals and their expected outcomes. The program parses these files, executes the solver for each scenario, and compares the results with the provided expectations.

### Input File Format

The input files use a specific format with keywords to define the test parameters for each deal. The key components are:

- **`NUMBER`**: An identifier for the test case.
- **`PBN`**: The deal definition in Portable Bridge Notation, describing the cards held by each of the four players (North, East, South, West).
- **`FUT`**: The expected number of future tricks for each player in different trump suits.
- **`TABLE`**: A table representing the number of tricks that can be taken by each partnership for each of the 13 possible opening leads.
- **`PAR`**: The par contract score for the deal, which is the best possible result achievable with optimal play from both sides.
- **`PLAY`**: A specific sequence of cards to be played, used to test the solver's analysis of a particular line of play.
- **`TRACE`**: Used to test the solver's trace functionality, which details the analysis at each step of a hand.

## Usage

The `dtest` program is typically run from the command line, taking an input file from the `hands` directory as an argument. It processes the test cases within the file and reports any discrepancies between the solver's output and the expected results.

For example:

```bash
./dtest ../hands/list1.txt
```

This command would run the test cases defined in `list1.txt` and check the DDS library's calculations.