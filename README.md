> _Academic project developed as part of a course at university._
# Bibliotecă stdio personalizată (SO_FILE)

Reimplementare în C a unei părți din biblioteca standard `stdio.h`, folosită pentru operații pe fișiere prin buffering.

## Structura `SO_FILE`

```c
typedef struct {
    int   fd;                  // file descriptor-ul fișierului
    char  mode[2];             // modul de deschidere ("r", "w", "a", etc.)
    char  buffer[BUFF_SIZE];   // buffer pentru citire / scriere
    int   buffer_size;         // dimensiunea efectivă a datelor din buffer
    int   cursor;              // poziția curentă în fișier
    int   error;               // flag de eroare
    int   current_buffer_pos;  // poziția curentă în buffer
    int   endOfFile;           // flag pentru EOF
    int   flag_op;             // ultima operație efectuată (vezi mai jos)
    pid_t childPid;            // PID-ul procesului copil (pentru popen/pclose)
} SO_FILE;
```

### Constante pentru `flag_op`

| Macro       | Valoare | Semnificație                                    |
| ----------- | :-----: | ----------------------------------------------- |
| `OP`        |   `0`   | Orice altă operație în afară de READ și WRITE   |
| `READ_OP`   |   `1`   | Ultima operație a fost de citire                |
| `WRITE_OP`  |   `2`   | Ultima operație a fost de scriere               |

---

## API

### 1. `SO_FILE *so_fopen(const char *pathname, const char *mode);`
Deschide fișierul în modul specificat și alocă dinamic o structură `SO_FILE`, populând-o cu informațiile necesare (fd, mod, cursor, etc.) și inițializând restul câmpurilor cu `0`.

**Returnează:** pointer către `SO_FILE` la succes, `NULL` la eroare.

---

### 2. `int so_fclose(SO_FILE *stream);`
Eliberează resursele asociate `stream`-ului. Dacă ultima operație a fost de **scriere**, apelează `so_fflush` pentru a goli bufferul în fișier înainte de închidere, evitând pierderea de date.

**Returnează:** `0` la succes.

---

### 3. `int so_fgetc(SO_FILE *stream);`
Citește un singur caracter folosind bufferul intern. Dacă bufferul e gol sau s-a ajuns la finalul lui, se reinițializează și se citește un nou bloc din fișier. Actualizează pozițiile în buffer și în fișier.

**Returnează:** caracterul ca `unsigned char` (pentru a evita semn negativ pe valori > 127), `SO_EOF` la eroare/EOF.

---

### 4. `int so_fputc(int c, SO_FILE *stream);`
Scrie un caracter în bufferul asociat fișierului. Dacă bufferul e plin, declanșează `so_fflush` pentru a-l elibera. Actualizează pozițiile în buffer și fișier.

**Returnează:** caracterul scris la succes, `SO_EOF` la eroare.

---

### 5. `size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream);`
Citește `nmemb` elemente a câte `size` octeți folosind `so_fgetc` octet cu octet.
- Bucla exterioară parcurge cele `nmemb` elemente.
- Bucla interioară citește cei `size` octeți ai fiecărui element.

**Returnează:** numărul de elemente citite cu succes.

---

### 6. `size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream);`
Simetric cu `so_fread`. Scrie `nmemb * size` octeți în buffer prin `so_fputc`.

**Returnează:** numărul de elemente scrise cu succes.

---

### 7. `int so_fseek(SO_FILE *stream, long offset, int whence);`
Mută cursorul în fișier.
- Dacă ultima operație a fost **WRITE** → se face flush pentru sincronizarea datelor.
- Dacă ultima operație a fost **READ** → bufferul și poziția în el se resetează.
- Poziționarea efectivă se face cu `lseek`.

**Returnează:** `0` la succes, `-1` la eroare.

---

### 8. `long so_ftell(SO_FILE *stream);`
Returnează poziția curentă a cursorului în fișier, folosind `so_fseek` cu `SEEK_CUR` și offset `0`.

---

### 9. `int so_fflush(SO_FILE *stream);`
Dacă ultima operație a fost de **scriere**, forțează scrierea conținutului bufferului în fișier, apoi golește bufferul și resetează variabilele asociate.

**Returnează:** `0` la succes.

---

### 10. `SO_FILE *so_popen(const char *command, const char *type);`
Pornește un proces copil care execută comanda dată și creează un pipe pentru comunicarea părinte–copil.

| `type` | Comportament                                                                                       |
| :----: | -------------------------------------------------------------------------------------------------- |
|  `r`   | `stdin` în copil e redirectat din pipe — copilul citește ce scrie părintele                        |
|  `w`   | `stdout` în copil e redirectat în pipe — părintele citește ce scrie copilul                        |

PID-ul copilului e salvat în `childPid` pentru a fi așteptat ulterior.

---

### 11. `int so_pclose(SO_FILE *stream);`
Eliberează resursele cu `so_fclose`, apoi așteaptă terminarea procesului copil (`waitpid`).

**Returnează:** statusul de terminare al procesului copil.

---

## Note de implementare

- Toate operațiile sunt **buffered** — datele trec prin `buffer[BUFF_SIZE]` înainte de a ajunge la `read(2)` / `write(2)`.
- Câmpul `flag_op` permite detectarea trecerii citire → scriere (și invers), pentru a face flush sau a invalida bufferul.
- Citirea returnează `unsigned char` pentru a nu confunda octeții cu valoarea negativă `EOF`.
- `so_popen` / `so_pclose` folosesc `fork(2)` + `execlp(2)` + `pipe(2)` + `dup2(2)`.

---
