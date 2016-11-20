# P2
Pracownia 2 - Systemy Operacyjne

## Temat 4
### Prosty program współbieżny zarządzający żądaniami dysku

Planista dysku otrzymuje i przydziela I/O dysku wielu wątkom. Wątki wysyłają żądania dostępu do dysku poprzez kolejkowanie ich w planiście dysku. Kolejka planisty ma ograniczoną pojemność, więc wątki muszą oczekiwać, gdy kolejka jest pełna./n

Program przy starcie tworzy określoną liczbę wątków żądających dostępu do dysku i jeden do ich obsługi. Każdy z wątków wysyła serię żądań dostępu do ścieżek dysku (określonych w pliku wejściowym). Wątek żądający dostępu musi poczekać, aż wątek obsługujący żądania skończy obsługiwać wcześniejsze żądanie tego wątku. Wątek kończy się, gdy wszystkie jego żądania zostaną obsłużone./n

Żądania są kolejkowane w porządku SSTF (następne obsługiwane żądanie dostępu znajduje się najbliżej aktualnej ścieżki). Początkowa ścieżka dysku to 0./n

*Źródło: <https://people.cs.umass.edu/~mcorner/courses/691J/project1.text>*