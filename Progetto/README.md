# UNIVERSITY

## Installation

1. Clone the repository:
    ```
    git clone https://github.com/agambalonga/Reti_di_calcolatori.git
    ```
2. Navigate into the project folder:
    ```
    cd Progetto
    ```
3. Compile the source files in the `src` folder:
    ```
    gcc -o server_universitario src/server_universitario.c
    gcc -o segreteria src/segreteria.c
    gcc -o studente src/studente.c
    ```

## Usage

1. Start the server:
    ```
    ./server_universitario
    ```
2. In another terminal, start the secretary:
    ```
    ./segreteria 127.0.0.1
    ```
3. In another terminal, start the student:
    ```
    ./studente
    ```

### Secretary

The secretary can perform the following operations:

- Add new exams to the university server.
- Forward student exam booking requests to the university server.
- Provide students with the available exam dates for the chosen exam.

### Student

The student can perform the following operations:

- Ask the secretary if there are available exams for a course.
- Send an exam booking request to the secretary.

### University Server

The university server can perform the following operations:

- Receive the addition of new exams.
- Receive an exam booking.
