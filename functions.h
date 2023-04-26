#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>
#include <syslog.h>

/*
        OPIS ZMIENNYCH GLOBLANYCH
- `recursive` jest zmienną typu `bool`, która przechowuje informację, czy synchronizacja ma być rekurencyjna czy nie. Domyślnie ustawiona jest na `false`.

- `timeSleep` jest zmienną typu `unsigned long`, która przechowuje czas w sekundach, jaki demon ma odczekać między kolejnymi synchronizacjami. Domyślnie ustawiona jest na 5 minut (czyli 5 * 60 sekund).

- `forcedSynchro` jest zmienną typu `bool`, która przechowuje informację, czy synchronizacja została wymuszona przez sygnał `SIGUSR1`. Domyślnie ustawiona jest na `false`.

- `mmapThreshold` jest zmienną typu `int`, która przechowuje wartość graniczną w MB dla plików, które zostaną podzielone na mniejsze fragmenty podczas kopiowania. Domyślnie ustawiona jest na wartość `100`, co oznacza, że pliki o rozmiarze powyżej 100MB zostaną podzielone na mniejsze fragmenty.

- `mmapThreshold_DEFAULT` jest stałą, która definiuje domyślną wartość dla zmiennej `mmapThreshold`.

- `MAX_COPYING_BUFFER_SIZE` jest stałą, która definiuje maksymalny rozmiar bufora do kopiowania danych podczas synchronizacji. Domyślnie ustawiona jest na `65536` bajtów (czyli 64KB).
*/
bool recursive = false;
unsigned long timeSleep = 5 * 60;
bool forcedSynchro = false;
#define mmapThreshold_DEFAULT 100
int mmapThreshold = mmapThreshold_DEFAULT;
#define MAX_COPYING_BUFFER_SIZE 65536

/*
Funkcja "changeModTime" jest używana do zmiany czasu modyfikacji pliku na aktualny czas systemowy. Przyjmuje ona jako argument wskaźnik na łańcuch znaków reprezentujący ścieżkę do pliku, którego czas modyfikacji ma zostać zmieniony.

Funkcja ta korzysta z funkcji systemowej "time", która zwraca aktualny czas systemowy w formacie czasu uniksowego (liczba sekund od 1 stycznia 1970 roku). Następnie tworzy strukturę "utimbuf", która służy do przechowywania czasu dostępu i modyfikacji pliku. W tym przypadku, czas dostępu nie jest zmieniany, więc zostaje ustawiony na aktualny czas systemowy. Czas modyfikacji zostaje również ustawiony na aktualny czas systemowy.

Na koniec, funkcja wywołuje funkcję systemową "utime", która przyjmuje jako argumenty ścieżkę do pliku i strukturę "utimbuf" z ustawionymi czasami dostępu i modyfikacji. Po wywołaniu tej funkcji czas modyfikacji pliku zostanie zmieniony na aktualny czas systemowy.
*/
void changeModTime(char *srcFilePath);

/*
Funkcja "currentTime" służy do wyświetlania aktualnego czasu systemowego w formacie "rok-miesiąc-dzień godzina:minuty:sekundy".

Funkcja ta korzysta z funkcji systemowej "time", która zwraca aktualny czas systemowy w formacie czasu uniksowego (liczba sekund od 1 stycznia 1970 roku). Następnie korzysta z funkcji "localtime", która konwertuje czas uniksowy na lokalny czas, w zależności od ustawień strefy czasowej w systemie. Wynikiem tej funkcji jest struktura "tm", która zawiera informacje o roku, miesiącu, dacie, godzinie, minutach i sekundach.

Funkcja "strftime" jest używana do formatowania czasu w odpowiedni sposób. W tym przypadku, formatowanie odbywa się za pomocą ciągu znaków "%Y-%m-%d %H:%M:%S", który oznacza kolejno: rok, miesiąc, dzień, godzinę, minutę i sekundę, oddzielone myślnikami i dwukropkami. Wynik formatowania zostaje zapisany w tablicy "datetime".

Na końcu, funkcja wyświetla sformatowany czas za pomocą funkcji "printf", którą przekazuje łańcuch znaków z formatowanym czasem wewnątrz nawiasów kwadratowych, umieszczając go wraz z dodanym przed nim nawiasem kwadratowym wewnątrz jednego z nawiasów okrągłych. Takie wyświetlenie pozwala na oznaczenie czasu jako znacznik czasowy w różnych kontekstach, na przykład w logach lub w konsoli systemowej.
*/
void currenTime();

/*
Funkcja "clearTheArray" oczyszcza zawartość tablicy znaków przekazanej jako argument do funkcji.

Argument "arr" to wskaźnik na początek tablicy znaków, którą chcemy wyczyścić.

Wewnątrz funkcji używana jest funkcja "strlen", która zwraca długość napisu, który przekazujemy jako argument (tablica znaków kończy się znakiem null, czyli '\0', więc długość napisu to liczba znaków przed tym znakiem).

Następnie wykorzystywana jest funkcja "memset", która ustawia kolejne bajty pamięci na wartość podaną jako drugi argument (w tym przypadku jest to znak null, czyli '\0'). Funkcja "memset" ustawia wartość w pamięci przez określoną liczbę bajtów, która jest wyliczana jako długość napisu pomnożona przez rozmiar jednego elementu tablicy (w tym przypadku rozmiar jednego znaku, czyli 1 bajt).

W rezultacie, po wywołaniu tej funkcji, tablica "arr" będzie zawierała same znaki null, co oznacza, że zostanie ona w pełni wyczyszczona.
*/
void clearTheArray(char *arr);

/*
Funkcja "copyDirectory" kopiuje zawartość katalogu źródłowego (srcPath) do katalogu docelowego (dstPath), włączając w to wszystkie pliki i podkatalogi. W przypadku katalogów, kopiowanie jest rekurencyjne, czyli funkcja wywołuje siebie sama dla każdego podkatalogu.

Funkcja używa biblioteki dirent.h do iteracji przez pliki i podkatalogi w katalogu źródłowym. W każdej iteracji, funkcja sprawdza, czy bieżący plik jest katalogiem, czy plikiem, a następnie wykonuje odpowiednie działania.

Jeśli bieżący plik to plik, funkcja porównuje czas modyfikacji pliku źródłowego i docelowego, aby określić, czy plik źródłowy został zmieniony. Jeśli tak, funkcja kopiowania pliku zostaje wywołana, a czas modyfikacji pliku źródłowego jest ustawiany na czas bieżący. W przeciwnym razie funkcja wyświetla odpowiedni komunikat.

Jeśli bieżący plik to katalog, funkcja wywołuje samą siebie dla katalogu źródłowego i docelowego, aby skopiować zawartość katalogu.

Funkcja również obsługuje błędy, takie jak nieudane otwarcie katalogu źródłowego lub nieudane utworzenie katalogu docelowego. Komunikaty o błędach są wyświetlane na ekranie, a program kończy działanie z kodem błędu.
*/
void copyDirectory(const char *srcPath, const char *dstPath);

/*
Funkcja "syncDirectory" synchronizuje zawartość dwóch katalogów - "srcPath" i "dstPath". Funkcja sprawdza, czy każdy plik i katalog w "dstPath" istnieje w "srcPath", a jeśli nie, to usuwa go z "dstPath". Jeśli plik lub katalog istnieje w obu katalogach, funkcja rekurencyjnie wywołuje się na nich, aby synchronizować ich zawartość.

W szczególności funkcja wykonuje następujące czynności:
- Otwiera katalog docelowy ("dstPath") za pomocą funkcji opendir.
- Iteruje po każdym pliku i katalogu w "dstPath" za pomocą readdir.
- Sprawdza, czy dany plik lub katalog to "." lub "..", które są specjalnymi katalogami systemowymi i pomija je.
- Tworzy ścieżki źródłowe i docelowe dla każdego pliku lub katalogu za pomocą snprintf.
- Sprawdza, czy plik lub katalog w "dstPath" jest plikiem regularnym lub katalogiem za pomocą lstat.
- Jeśli plik regularny nie istnieje w "srcPath", usuwa go z "dstPath" za pomocą unlink.
- Jeśli katalog nie istnieje w "srcPath", usuwa go z "dstPath". Jeśli katalog nie jest pusty, usuwa jego zawartość rekurencyjnie za pomocą rekurencyjnego wywołania syncDirectory z pustym "srcPath". Następnie usuwa pusty katalog za pomocą rmdir.
- Jeśli plik lub katalog istnieje w obu katalogach, rekurencyjnie wywołuje syncDirectory dla ścieżek źródłowej i docelowej.
- Zamyka katalog docelowy za pomocą closedir.

W przypadku wystąpienia błędu podczas otwierania, odczytywania lub zamykania katalogu, usuwania pliku lub katalogu lub wywoływania innych funkcji systemowych, funkcja wypisuje odpowiedni komunikat o błędzie i kończy działanie programu za pomocą exit.
*/
void syncDirectory(const char *srcPath, const char *dstPath);

/*
Funkcja "copy" ma trzy argumenty wejściowe: wskaźniki do łańcuchów znaków "source" i "destination", oraz "mmapThreshold", który określa maksymalny rozmiar pliku, dla którego funkcja będzie używać pamięci mapowanej w celu kopiowania.

Wewnątrz funkcji, na początku, jest wywoływana funkcja "stat" z argumentem "source", aby uzyskać informacje o pliku, takie jak jego rozmiar i właściciel. Jeśli "stat" zwróci wartość różną od zera, funkcja wypisze komunikat "Failed on stat" i zakończy działanie.

Następnie, rozmiar pliku jest przypisywany do zmiennej "fileSize", a zmienna "status" jest zainicjowana.

Jeśli rozmiar pliku jest większy niż "mmapThreshold", funkcja wywoła funkcję "copyUsingMMapWrite" z argumentami "source", "destination" i "fileSize", która kopiuje plik z użyciem pamięci mapowanej. W przeciwnym razie, funkcja wywoła funkcję "copyUsingReadWrite" z takimi samymi argumentami, która kopiuje plik przy użyciu operacji odczytu i zapisu.

Na końcu, jeśli zmienna "status" ma wartość inną niż "EXIT_SUCCESS", funkcja zwróci wartość tej zmiennej. W przeciwnym razie, funkcja zwróci "EXIT_SUCCESS".
*/
int copy(char *source, char *destination, int mmapThreshold);

/*
Funkcja "copyUsingReadWrite" służy do kopiowania pliku z użyciem operacji odczytu i zapisu. Ma trzy argumenty wejściowe: łańcuchy znaków "srcPath" i "dstPath", które określają ścieżki plików źródłowego i docelowego odpowiednio, oraz "bufferSize", który określa rozmiar bufora, który będzie używany podczas kopiowania pliku.

Wewnątrz funkcji, na początku, otwierane są pliki źródłowy i docelowy z użyciem funkcji "open". W przypadku niepowodzenia otwarcia pliku źródłowego, funkcja wypisze komunikat "Error opening source file." i zakończy działanie. W przypadku niepowodzenia otwarcia pliku docelowego, funkcja wypisze komunikat "Target file open error." i zakończy działanie.

Następnie, jest tworzony bufor o rozmiarze "bufferSize". W pętli "while", jest wywoływana funkcja "read", aby odczytać dane z pliku źródłowego i zapisać je w buforze. Jeśli odczytanych zostanie 0 bajtów, pętla zostanie przerwana. Następnie, funkcja "write" jest wywoływana, aby zapisać dane z bufora do pliku docelowego. Jeśli zapisane zostanie mniej bajtów niż odczytane, funkcja wypisze komunikat "Error writing to target file." i zakończy działanie.

Na końcu, funkcja sprawdza, czy ilość odczytanych danych jest równa -1. Jeśli tak, oznacza to, że wystąpił błąd podczas odczytu danych z pliku źródłowego, a funkcja wypisze komunikat "Source file read error." i zakończy działanie.

Na końcu, funkcja zamyka pliki źródłowy i docelowy przy użyciu funkcji "close". Jeśli którykolwiek z plików nie zostanie pomyślnie zamknięty, funkcja wypisze komunikat "File close error." i zakończy działanie.*/
void copyUsingReadWrite(const char *srcPath, const char *dstPath, long int bufferSize);

/*
Funkcja "copyUsingMMapWrite" ma na celu skopiowanie zawartości pliku o nazwie "source" do pliku o nazwie "destination" przy użyciu mapowania pamięci.

Funkcja otwiera pliki źródłowy i docelowy przy użyciu funkcji open(), zwracając wartość EXIT_FAILURE w przypadku niepowodzenia. Następnie funkcja używa funkcji ftruncate() do ustawienia rozmiaru pliku docelowego na rozmiar pliku źródłowego, co oznacza, że ​​docelowy plik będzie miał ten sam rozmiar co źródłowy plik.

Następnie funkcja używa funkcji mmap() w celu mapowania pamięci źródłowej i docelowej plików. Mapowanie pamięci jest techniką, która umożliwia aplikacjom dostęp do plików jak do pamięci wirtualnej, umożliwiając efektywną wymianę danych między plikami a pamięcią systemu.

Funkcja używa memcpy() do skopiowania zawartości pliku źródłowego do pliku docelowego. Następnie funkcja używa munmap() do zwolnienia mapowanych obszarów pamięci i zamyka pliki źródłowy i docelowy przy użyciu funkcji close().

Funkcja zwraca wartość EXIT_SUCCESS, gdy skopiowanie zostanie wykonane pomyślnie, a wartość EXIT_FAILURE, gdy wystąpi błąd.
*/
int copyUsingMMapWrite(char *source, char *destination, long int fileSize);

/*
Funkcja "compareDestSrc" ma na celu porównanie plików znajdujących się w dwóch katalogach - źródłowym i docelowym - i usunięcie plików, które znajdują się w katalogu docelowym, ale nie w katalogu źródłowym.

Funkcja rozpoczyna się od otwarcia katalogów źródłowego i docelowego przy użyciu funkcji opendir(). Następnie funkcja sprawdza, czy katalog źródłowy i docelowy zostały poprawnie otwarte. Jeśli któryś z nich nie został poprawnie otwarty, funkcja wyświetli komunikat o błędzie i zwróci sterowanie.

Funkcja następnie przechodzi przez wszystkie wpisy w katalogu docelowym przy użyciu funkcji readdir(). Dla każdego wpisu funkcja sprawdza, czy jest to plik regularny (DT_REG) - jeśli tak, to funkcja buduje pełną ścieżkę do pliku źródłowego i sprawdza, czy plik istnieje przy użyciu funkcji access(). Jeśli plik nie istnieje, funkcja buduje pełną ścieżkę do pliku docelowego i usuwa go przy użyciu funkcji unlink(). Funkcja wyświetla informację o usunięciu pliku i datę usunięcia za pomocą funkcji currenTime().

Funkcja kończy się przez zamknięcie katalogów źródłowego i docelowego przy użyciu funkcji closedir().
*/
void compareDestSrc(char *sourcePath, char *destinationPath);

/*
Funkcja "compareSrcDest" porównuje zawartość dwóch katalogów o ścieżkach `sourcePath` i `destinationPath`.

Najpierw otwiera oba katalogi przy użyciu funkcji `opendir()`. Następnie sprawdza, czy otwarcie katalogu `sourcePath` zakończyło się sukcesem. Jeśli nie, funkcja wyświetla informację o błędzie i kończy swoje działanie. Jeśli otwarcie katalogu `destinationPath` zakończyło się niepowodzeniem, funkcja wyświetla informację o błędzie i kończy swoje działanie, po uprzednim zamknięciu katalogu `sourceDir`.

Funkcja korzysta z biblioteki `dirent.h` do przechodzenia przez zawartość katalogów. Iteruje po zawartości katalogu `sourcePath`, odczytując po kolei jego elementy. Jeśli aktualny element jest plikiem zwykłym, funkcja sprawdza, czy istnieje plik o tej samej nazwie w katalogu `destinationPath`. Jeśli taki plik istnieje, funkcja wyświetla informację o znalezieniu pliku o tej samej nazwie, a następnie odczytuje czas ostatniej modyfikacji pliku z katalogu `sourcePath` i `destinationPath`. Jeśli czas ostatniej modyfikacji tych plików jest różny, funkcja kopiuję plik ze ścieżki `sourcePath` do ścieżki `destinationPath` przy użyciu funkcji `copy()`. Następnie ustawia czas ostatniej modyfikacji skopiowanego pliku na czas ostatniej modyfikacji pliku z `sourcePath`. Jeśli plik o tej samej nazwie nie istnieje w katalogu `destinationPath`, funkcja wyświetla informację o nie znalezieniu pliku i kopiuje go ze ścieżki `sourcePath` do ścieżki `destinationPath`.

Funkcja `currenTime()` jest używana do wyświetlenia bieżącej daty i czasu, a funkcja `clearTheArray()` jest używana do wyczyszczenia tablicy znaków.

Funkcja zwraca `void`.
*/
void compareSrcDest(char *sourcePath, char *destinationPath);

/*
Funkcja "recursiveSynchronization" przyjmuje dwa argumenty typu char*, które reprezentują ścieżki do katalogów, które mają zostać zsynchronizowane.

Funkcja wywołuje dwie inne funkcje: "copyDirectory" i "syncDirectory". Pierwsza z nich kopiuje zawartość katalogu źródłowego do katalogu docelowego, natomiast druga synchronizuje zawartość katalogu źródłowego i katalogu docelowego, czyli aktualizuje pliki w katalogu docelowym, które zostały zmienione w katalogu źródłowym od czasu ostatniej synchronizacji.

Cała funkcja działa rekurencyjnie, co oznacza, że jeśli w katalogu źródłowym znajdują się inne katalogi, to również zostaną one zsynchronizowane z odpowiadającymi katalogami w katalogu docelowym.

W ogólnym zarysie, funkcja ta ma na celu zsynchronizowanie dwóch katalogów, aby zawierały one identyczną zawartość, uwzględniając zmiany, które mogły wystąpić w międzyczasie.*/
void recursiveSynchronization(char *srcPath, char *dstPath);

/*
Funkcja "Demon" przyjmuje tablicę dwóch łańcuchów znaków argv, które reprezentują ścieżkę do źródłowego katalogu (argv[1]) i docelowego katalogu (argv[2]).

W funkcji, najpierw sprawdzane jest czy zmienna "recursive" jest ustawiona na wartość prawda (true). Jeśli tak, wywoływana jest funkcja recursiveSynchronization(), która synchronizuje zawartość źródłowego katalogu z docelowym katalogiem, poprzez skopiowanie plików i katalogów z jednego do drugiego i zaktualizowanie ich zawartości.

Jeśli zmienna "recursive" nie jest ustawiona na wartość prawda, wywoływana jest funkcja compareSrcDest(), która porównuje zawartość źródłowego katalogu z zawartością docelowego katalogu i wyświetla informacje o ewentualnych różnicach między nimi.

W skrócie, funkcja Demon jest punktem wejścia do programu i decyduje, której z dwóch funkcji (recursiveSynchronization() lub compareSrcDest()) należy użyć w zależności od wartości zmiennej "recursive".
*/
void Demon(char **argv);

/*
Funkcja "options" ma na celu analizę argumentów wejściowych przekazanych do programu i ustalenie wartości zmiennych globalnych, które będą wykorzystywane w innych funkcjach. Funkcja przyjmuje dwa argumenty: "argc" - ilość argumentów przekazanych do programu oraz "argv" - tablica napisów, która przechowuje argumenty.

Funkcja rozpoczyna pętlę "for" od indeksu 3, ponieważ argumenty przekazane do programu o indeksach 0, 1 i 2 zawierają informacje o nazwie programu, źródłowym i docelowym katalogu.

Wewnątrz pętli sprawdzane są kolejne argumenty za pomocą funkcji "strcmp", która porównuje napisy. Jeśli argument odpowiada jednemu z trzech flag: "-r", "-t" lub "-d", to zostaje przypisana odpowiednia wartość do zmiennych globalnych "recursive", "timeSleep" lub "mmapThreshold".

W przypadku flagi "-r", zmienna "recursive" jest ustawiana na wartość "true", co oznacza, że synchronizacja będzie odbywać się rekurencyjnie.

W przypadku flagi "-t", wartość po niej oznacza liczbę sekund, po której program będzie próbował ponownie wybudzic sie ze snu. Wartość ta zostanie przypisana do zmiennej "timeSleep".

W przypadku flagi "-d", wartość po niej oznacza maksymalny rozmiar pliku, który będzie mapowany w pamięci przy użyciu funkcji "mmap". Wartość ta zostanie przypisana do zmiennej "mmapThreshold".

Funkcja nie zwraca żadnej wartości, a jedynie ustawia wartości zmiennych globalnych.
*/
void options(int argc, char **argv);

/*
Funkcja "sigusr1_handler" obslubuje sygnał SIGUSR1, czyli sygnału użytkownika nr 1. Kiedy proces otrzymuje ten sygnał, system wywołuje tę funkcję, która wypisuje bieżący czas oraz komunikat "Demon awakening by signal SIGUSR1." Następnie zmienna forcedSynchro jest ustawiana na wartość true, co oznacza, że proces zostanie wymuszony do natychmiastowej synchronizacji w czasie kolejnej iteracji.
*/
void sigusr1_handler(int signum);

/*
Funkcja "createDemon" tworzy proces demona w systemie operacyjnym. Kod jest napisany w języku C.

Funkcja najpierw otwiera systemowy dziennik zdarzeń za pomocą funkcji "openlog", z ustawieniami, które powodują wyświetlenie identyfikatora procesu (PID) i identyfikatora użytkownika (UID) w logach.

Następnie funkcja wywołuje funkcję "fork", aby utworzyć nowy proces. Proces potomny jest utworzony i kod kontynuuje jego wykonanie, podczas gdy proces rodzicielski kończy swoje działanie za pomocą funkcji "exit". Proces potomny będzie działał jako demon.

Proces demon wywołuje funkcję "umask", aby ustawić maskę uprawnień plików na 0, co oznacza, że ​​tworzone pliki będą miały pełne uprawnienia.

Następnie demon wywołuje funkcję "setsid", aby utworzyć nową sesję. Funkcja ta powoduje, że proces staje się liderem nowej sesji, procesu grupowego i zrywa związek z terminalami kontrolnymi, co oznacza, że ​​demon nie jest już zależny od terminala.

Funkcja "currentTime" wywołuje inną funkcję, która zwraca aktualny czas, który jest wyświetlany na konsoli i zapisywany w dzienniku systemowym.

Następnie demon wyświetla swoje PID na konsoli i zapisuje go w dzienniku systemowym za pomocą funkcji "syslog".

Na koniec demon zamyka standardowe wejście, standardowe wyjście i standardowe wyjście błędów, aby zapobiec niechcianym wyjściom, a następnie zamyka dziennik zdarzeń za pomocą funkcji "closelog".
*/
void createDemon();

/*
Funkcja `main` jest punktem wejścia programu. Przyjmuje dwa argumenty, `argc`, który jest liczbą argumentów wiersza poleceń przekazanych do programu i `argv`, który jest tablicą łańcuchów zawierających argumenty wiersza poleceń.

Funkcja najpierw sprawdza, czy liczba argumentów jest mniejsza niż trzy, czyli minimalna liczba argumentów wymagana do poprawnego działania programu. Jeśli argumentów jest mniej, na konsoli wyświetlany jest komunikat o błędzie. W przeciwnym razie program kontynuuje ustawianie opcji i obsługę sygnału.

Funkcja `options` jest wywoływana w celu przeanalizowania argumentów wiersza poleceń i ustawienia odpowiednich opcji na podstawie ich wartości. Funkcja `signal` jest wywoływana w celu zarejestrowania procedury obsługi sygnału dla sygnału `SIGUSR1`, który jest używany do wymuszenia synchronizacji między katalogami źródłowym i docelowym.

Następnie funkcja drukuje bieżący czas i identyfikator procesu demona.

Rozpoczyna się pętla, która działa w nieskończoność. Jeśli `forcedSynchro` ma wartość false, funkcja drukuje komunikat wskazujący, że demon się obudził. Następnie wywoływana jest funkcja `Demon` w celu wykonania synchronizacji między katalogami źródłowym i docelowym.

Po zakończeniu synchronizacji funkcja wyświetla komunikat wskazujący, że demon śpi i czeka przez liczbę sekund określoną przez `timeSleep` przed rozpoczęciem kolejnej iteracji pętli. Jeśli flaga `forcedSynchro` jest prawdziwa, demon pomija krok synchronizacji i wraca do snu.
*/
int main(int argc, char **argv);