#include "functions.h"

void changeModTime(char *srcFilePath)
{
	// Przypisz ścieżkę pliku do zmiennej file_path.
	char *file_path = srcFilePath;

	// Pobierz aktualny czas i przypisz go do zmiennej now.
	time_t now = time(NULL);

	// Utwórz strukturę utimbuf i przypisz do jej pól czas dostępu i modyfikacji.
	struct utimbuf new_times;
	new_times.actime = now;	 // czas dostępu - zostawiamy bieżący
	new_times.modtime = now; // czas modyfikacji - ustawiamy na bieżący

	// Wywołaj funkcję utime z parametrami ścieżki pliku i nowych czasów.
	utime(file_path, &new_times);
}

void currenTime()
{
	// otwarcie systemowego logu z tagiem "Demon", numerem PID procesu oraz identyfikatorem użytkownika LOG_USER
	openlog("Demon", LOG_PID, LOG_USER);

	// pobranie aktualnego czasu systemowego w sekundach
	time_t current_time = time(NULL);

	// przetworzenie czasu do lokalnej strefy czasowej
	struct tm *local_time = localtime(&current_time);

	// utworzenie łańcucha znaków datetime zawierającego datę i godzinę w formacie YYYY-MM-DD HH:MM:SS
	char datetime[21];
	strftime(datetime, 21, "%Y-%m-%d %H:%M:%S", local_time);

	// zapisanie logu do systemowego logu z priorytetem LOG_INFO, zawierającego datę i godzinę w takim samym formacie jak na wyjściu
	// syslog(LOG_INFO, "[%s] ", datetime);

	// wyświetlenie daty i godziny w takim samym formacie jak na wyjściu na standardowym wyjściu
	printf("[%s] ", datetime);

	// zamknięcie systemowego logu
	closelog();
}

void clearTheArray(char *arr)
{
	// obliczenie długości łańcucha znaków przy pomocy funkcji strlen()
	size_t length = strlen(arr);

	// wyczyszczenie całego łańcucha znaków przy pomocy funkcji memset(), która wypełnia pierwsze length bajtów wskaźnika arr wartością '\0' (null terminator)
	memset(arr, '\0', length);
}

void copyDirectory(const char *srcPath, const char *dstPath)
{
	// Uruchomienie systemowego dziennika zdarzeń
	openlog("Demon", LOG_PID, LOG_USER);
	// Otwarcie katalogu źródłowego
	DIR *srcDirectory = opendir(srcPath);
	// Sprawdzenie czy udało się otworzyć katalog źródłowy
	if (srcDirectory == NULL)
	{
		// Funkcja wyświetlająca aktualny czas
		currenTime();
		printf("Błąd podczas otwierania katalogu źródłowego.");
		// Dodanie wpisu do dziennika zdarzeń
		syslog(LOG_ERR, "Błąd podczas otwierania katalogu źródłowego.");
		// Zakończenie programu z kodem błędu
		exit(EXIT_FAILURE);
	}
	if (mkdir(dstPath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1 && errno != EEXIST)
	{ // Sprawdzenie czy udało się utworzyć katalog docelowy
		currenTime();
		printf("Błąd podczas tworzenia katalogu docelowego.");
		syslog(LOG_ERR, "Błąd podczas tworzenia katalogu docelowego.");
		exit(EXIT_FAILURE);
	}
	// Struktura przechowująca informacje o plikach/katalogach w katalogu źródłowym
	struct dirent *srcFileInfo;
	// Pętla przechodząca przez wszystkie pliki/katalogi w katalogu źródłowym
	while ((srcFileInfo = readdir(srcDirectory)) != NULL)
	{
		// Ignorowanie katalogów "." i ".."
		if (strcmp(srcFileInfo->d_name, ".") == 0 || strcmp(srcFileInfo->d_name, "..") == 0)
		{
			continue;
		}
		// Zmienna przechowująca pełną ścieżkę do pliku/katalogu w katalogu źródłowym
		char srcFilePath[PATH_MAX];
		// Zmienna przechowująca pełną ścieżkę do pliku/katalogu w katalogu docelowym
		char dstFilePath[PATH_MAX];
		// Nazwa pliku/katalogu
		char *src = srcFileInfo->d_name;

		// Utworzenie pełnej ścieżki do pliku/katalogu źródłowego
		snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, srcFileInfo->d_name);
		// Utworzenie pełnej ścieżki do pliku/katalogu docelowego
		snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, srcFileInfo->d_name);

		// Struktura przechowująca informacje o pliku/katalogu źródłowym
		struct stat srcFileInfo;
		// Struktura przechowująca informacje o pliku/katalogu docelowym
		struct stat dstFileInfo;
		// Zmienna przechowująca czas modyfikacji pliku/katalogu źródłowego
		char modTimeSrc[20];
		// Zmienna przechowująca czas modyfikacji pliku/katalogu docelowego
		char modTimeDst[20];

		// Sprawdzenie statystyk pliku lub katalogu źródłowego i zapisanie ich w strukturze srcFileInfo
		if (lstat(srcFilePath, &srcFileInfo) == -1)
		{
			// Wyświetlenie komunikatu o błędzie na standardowym wyjściu oraz w logu systemowym
			currenTime();
			printf("Błąd odczytu statystyk pliku/katalogu źródłowego.");
			syslog(LOG_ERR, "Błąd odczytu statystyk pliku/katalogu źródłowego.");
			// Wyświetlenie ścieżki do pliku/katalogu źródłowego w logu systemowym
			syslog(LOG_ERR, "%s\n", srcFilePath);
			// Wyświetlenie ścieżki do pliku/katalogu docelowego w logu systemowym
			syslog(LOG_ERR, "%s\n", dstFilePath);
			// Wyjście z programu z kodem błędu
			exit(EXIT_FAILURE);
		}
		// Przekształcenie czasu modyfikacji pliku/katalogu źródłowego na format tekstowy
		strftime(modTimeSrc, sizeof(modTimeSrc), "%Y-%m-%d %H:%M:%S", localtime(&srcFileInfo.st_mtime));

		// Sprawdzamy czy istnieje plik docelowy
		if (lstat(dstFilePath, &dstFileInfo) == -1)
		{
			if (S_ISREG(srcFileInfo.st_mode))
			{ // plik
			  // Poniższy fragment kodu porównuje czas modyfikacji pliku źródłowego i docelowego
				// Jeśli są one różne, to następuje kopiowanie pliku, zmiana czasu modyfikacji, logowanie informacji o zdarzeniu oraz wypisanie na ekran informacji o zdarzeniu
				// Jeśli czas modyfikacji jest taki sam, to wypisywana jest informacja o tym, że znaleziono plik o takiej samej nazwie
				if (strcmp(modTimeSrc, modTimeDst) != 0)
				{
					copy(srcFilePath, dstFilePath, mmapThreshold);
					changeModTime(srcFilePath);
					currenTime();
					syslog(LOG_INFO, "Różne czasy modyfikacji: %s\n", src);
					printf("Różne czasy modyfikacji: %s\n", src);
					currenTime();
					syslog(LOG_INFO, "Plik: %s został pomyślnie skopiowany.\n", src);
					printf("Plik: %s został pomyślnie skopiowany.\n", src);
				}
				else
				{
					currenTime();
					syslog(LOG_INFO, "Znaleziono plik o takiej samej nazwie: %s\n", src);
					printf("Znaleziono plik o takiej samej nazwie: %s\n", src);
				}
			} // Jeśli plik jest katalogiem, to wywoływana jest funkcja kopiująca katalog, logowane jest zdarzenie oraz wypisywana informacja na ekranie
			else if (S_ISDIR(srcFileInfo.st_mode))
			{
				copyDirectory(srcFilePath, dstFilePath);
				currenTime();
				syslog(LOG_INFO, "Znaleziono katalog: %s\n", src);
				printf("Znaleziono katalog: %s\n", src);
			}
		}
		else
		{
			// Formatowanie czasu modyfikacji pliku docelowego
			strftime(modTimeDst, sizeof(modTimeDst), "%Y-%m-%d %H:%M:%S", localtime(&dstFileInfo.st_mtime));
			if (strcmp(modTimeSrc, modTimeDst) != 0)
			{
				if (S_ISREG(srcFileInfo.st_mode))
				{ // plik
					if (strcmp(modTimeSrc, modTimeDst) != 0)
					{
						copy(srcFilePath, dstFilePath, mmapThreshold);
						changeModTime(srcFilePath);
						currenTime();
						syslog(LOG_INFO, "Różne czasy modyfikacji: %s\n", src);
						printf("Różne czasy modyfikacji: %s\n", src);
						currenTime();
						syslog(LOG_INFO, "Plik: %s został pomyślnie skopiowany.\n", src);
						printf("Plik: %s został pomyślnie skopiowany.\n", src);
					}
					else
					{
						currenTime();
						printf("Znaleziono plik o takiej samej nazwie: %s\n", src);
						syslog(LOG_INFO, "Znaleziono plik o takiej samej nazwie: %s\n", src);
					}
				}
				else if (S_ISDIR(srcFileInfo.st_mode))
				{ // katalog
					copyDirectory(srcFilePath, dstFilePath);
					currenTime();
					printf("Znaleziono katalog: %s\n", src);
					syslog(LOG_INFO, "Znaleziono katalog: %s\n", src);
				}
			}
		}
	}

	// Jeśli zamknięcie katalogu źródłowego się nie powiedzie:
	if (closedir(srcDirectory) == -1)
	{
		// Wywołaj funkcję currenTime() - pobierająca aktualny czas.
		currenTime();
		// Wyświetl komunikat błędu w konsoli.
		printf("Błąd podczas zamykania katalogu źródłowego.");
		// Wywołaj funkcję syslog() - logowanie błędów systemowych.
		syslog(LOG_ERR, "Błąd podczas zamykania katalogu źródłowego.");
		// Zakończ program z kodem błędu.
		exit(EXIT_FAILURE);
	}

	closelog();
}

void syncDirectory(const char *srcPath, const char *dstPath)
{
	// Otwieranie logu systemowego z nazwą "Demon", z PID-em oraz użytkownikiem
	openlog("Demon", LOG_PID, LOG_USER);

	// Otwieranie katalogu docelowego za pomocą funkcji opendir()
	// Jeśli otwarcie katalogu się nie powiedzie, program zakończy działanie z kodem błędu
	DIR *dstDirectory = opendir(dstPath);
	if (dstDirectory == NULL)
	{
		// Wywołanie funkcji currenTime() do wypisania aktualnego czasu
		currenTime();
		printf("Błąd podczas otwierania katalogu docelowego.");
		// Zapisanie błędu do logu systemowego za pomocą funkcji syslog()
		syslog(LOG_ERR, "Błąd podczas otwierania katalogu docelowego.");
		exit(EXIT_FAILURE); // Zakończenie działania programu z kodem błędu
	}

	// Struktura przechowująca informacje o pliku/katalogu w katalogu docelowym
	struct dirent *FileOrDirectory;

	// Pętla while, która będzie wykonywać operacje dopóki kolejny element katalogu docelowego istnieje
	// readdir() zwróci NULL, kiedy wszystkie elementy zostaną przeczytane
	while ((FileOrDirectory = readdir(dstDirectory)) != NULL)
	{
		// Sprawdzanie, czy element katalogu nie jest "." ani ".."
		// Jeśli tak, to pomiń ten element i przejdź do kolejnego
		if (strcmp(FileOrDirectory->d_name, ".") == 0 || strcmp(FileOrDirectory->d_name, "..") == 0)
		{
			continue;
		}
		// Tworzenie zmiennych przechowujących ścieżki plików/katalogów źródłowych i docelowych
		char srcFilePath[PATH_MAX];
		char dstFilePath[PATH_MAX];
		// Używanie funkcji snprintf() do sklejenia ścieżki źródłowej i nazwy pliku/katalogu
		snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, FileOrDirectory->d_name);
		// Używanie funkcji snprintf() do sklejenia ścieżki docelowej i nazwy pliku/katalogu
		snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, FileOrDirectory->d_name);
		// Struktura przechowująca informacje o pliku/katalogu
		struct stat FileOrDirectoryInfo;

		// Pobieranie informacji o pliku/katalogu docelowym przy pomocy funkcji lstat()
		// Jeśli funkcja zwróci -1, to znaczy, że wystąpił błąd i program zakończy działanie z kodem błędu
		if (lstat(dstFilePath, &FileOrDirectoryInfo) == -1)
		{
			// Wywołanie funkcji currenTime() do wypisania aktualnego czasu
			currenTime();
			printf("Błąd podczas odczytywania statystyk pliku/katalogu docelowego.");
			// Zapisanie błędu do logu systemowego za pomocą funkcji syslog()
			syslog(LOG_ERR, "Błąd podczas odczytywania statystyk pliku/katalogu docelowego.");
			exit(EXIT_FAILURE); // Zakończenie działania programu z kodem błędu
		}

		// Sprawdzanie czy plik lub katalog istnieje w katalogu źródłowym i usuwanie, jeśli tak
		if (S_ISREG(FileOrDirectoryInfo.st_mode))
		{
			// Jeśli to plik regularny, sprawdź czy istnieje w katalogu źródłowym
			if (access(srcFilePath, F_OK) == -1)
			{
				// Usuwanie pliku
				currenTime();
				printf("Plik: %s został pomyślnie usunięty.\n", FileOrDirectory->d_name);
				syslog(LOG_INFO, "Plik: %s został pomyślnie usunięty.\n", FileOrDirectory->d_name);
				if (unlink(dstFilePath) == -1)
				{
					currenTime();
					printf("Błąd usuwania pliku docelowego.");
					syslog(LOG_ERR, "Błąd usuwania pliku docelowego.");
					exit(EXIT_FAILURE);
				}
			}
		}
		else if (S_ISDIR(FileOrDirectoryInfo.st_mode))
		{
			if (access(srcFilePath, F_OK) == -1)
			{
				// Usuwanie katalogu
				currenTime();
				printf("Katalog: %s został pomyślnie usunięty.\n", FileOrDirectory->d_name);
				syslog(LOG_INFO, "Katalog: %s został pomyślnie usunięty.\n", FileOrDirectory->d_name);
				if (rmdir(dstFilePath) == -1)
				{
					if (errno == ENOTEMPTY)
					{
						// Synchronizacja zawartości katalogu i usuwanie
						syncDirectory("", dstFilePath);
						if (rmdir(dstFilePath) == -1)
						{
							currenTime();
							printf("Błąd usuwania katalogu docelowego.");
							syslog(LOG_ERR, "Błąd usuwania katalogu docelowego.");
							exit(EXIT_FAILURE);
						}
					}
					else
					{
						currenTime();
						printf("Błąd usuwania katalogu docelowego.");
						syslog(LOG_ERR, "Błąd usuwania katalogu docelowego.");
						exit(EXIT_FAILURE);
					}
				}
			}
			else
			{
				// Synchronizacja zawartości katalogu
				syncDirectory(srcFilePath, dstFilePath);
			}
		}
	}

	// Zamykanie katalogu docelowego przy pomocy funkcji closedir()
	// Jeśli zamknięcie katalogu się nie powiedzie, program zakończy działanie z kodem błędu
	if (closedir(dstDirectory) == -1)
	{
		// Wywołanie funkcji currenTime() do wypisania aktualnego czasu
		currenTime();
		printf("Błąd podczas zamykania katalogu docelowego.");
		// Zapisanie błędu do logu systemowego za pomocą funkcji syslog()
		syslog(LOG_ERR, "Błąd podczas zamykania katalogu docelowego.");
		exit(EXIT_FAILURE); // Zakończenie działania programu z kodem błędu
	}

	closelog();
}

int copy(char *source, char *destination, int mmapThreshold)
{
	// Sprawdzenie informacji o pliku źródłowym
	struct stat fileStat;
	if (stat(source, &fileStat) == -1)
	{
		printf("Błąd podczas wykonywania funkcji stat.");
	}
	long int fileSize = fileStat.st_size;
	int status;

	// Sprawdzenie wielkości pliku i wybór metody kopiowania
	if (fileSize > mmapThreshold)
	{
		status = copyUsingMMapWrite(source, destination, fileSize);
	}
	else
	{
		copyUsingReadWrite(source, destination, fileSize);
	}

	// Obsługa błędów kopiowania
	if (status != EXIT_SUCCESS)
	{
		return status;
	}

	return EXIT_SUCCESS;
}

void copyUsingReadWrite(const char *srcPath, const char *dstPath, long int bufferSize)
{
	// Inicjalizacja demonowego logowania z identyfikatorem PID i źródłem logowania użytkownika
	openlog("Demon", LOG_PID, LOG_USER);

	// Otwarcie pliku źródłowego w trybie tylko do odczytu
	int srcFile = open(srcPath, O_RDONLY);
	if (srcFile == -1)
	{
		// Obsługa błędu w przypadku niepowodzenia otwarcia pliku źródłowego
		currenTime();
		printf("Błąd podczas otwierania pliku źródłowego.");
		syslog(LOG_ERR, "Błąd podczas otwierania pliku źródłowego.");
		exit(EXIT_FAILURE);
	}

	// Otwarcie pliku docelowego w trybie tylko do zapisu, tworząc go jeśli nie istnieje
	int dstFile = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (dstFile == -1)
	{
		// Obsługa błędu w przypadku niepowodzenia otwarcia pliku docelowego
		currenTime();
		printf("Błąd otwarcia pliku docelowego.");
		syslog(LOG_ERR, "Błąd otwarcia pliku docelowego.");
		exit(EXIT_FAILURE);
	}

	// Bufor dla danych z pliku źródłowego i zmienne do przechowywania liczby odczytanych i zapisanych bajtów
	char buf[bufferSize];
	ssize_t bytesRead, bytesWritten;

	// Pętla odczytuje dane ze źródłowego pliku i zapisuje je do docelowego pliku
	while ((bytesRead = read(srcFile, buf, sizeof(buf))) > 0)
	{
		bytesWritten = write(dstFile, buf, bytesRead);
		if (bytesWritten == -1)
		{
			currenTime();
			printf("Błąd zapisu do pliku docelowego.");
			syslog(LOG_ERR, "Błąd zapisu do pliku docelowego.");
			exit(EXIT_FAILURE);
		}
	}

	// Jeśli odczytanie danych ze źródłowego pliku zakończy się niepowodzeniem, to wyświetlamy odpowiedni komunikat błędu i kończymy działanie programu
	if (bytesRead == -1)
	{
		currenTime();
		printf("Błąd odczytu pliku źródłowego.");
		syslog(LOG_ERR, "Błąd odczytu pliku źródłowego.");
		exit(EXIT_FAILURE);
	}

	// Zamykamy otwarte pliki źródłowy i docelowy; jeśli zamknięcie plików zakończy się niepowodzeniem, to wyświetlamy odpowiedni komunikat błędu i kończymy działanie programu
	if (close(srcFile) == -1 || close(dstFile) == -1)
	{
		currenTime();
		printf("Błąd zamykania pliku.");
		syslog(LOG_ERR, "Błąd zamykania pliku.");
		exit(EXIT_FAILURE);
	}

	closelog();
}

int copyUsingMMapWrite(char *source, char *destination, long int fileSize)
{
	// Otwieranie logu systemowego z nazwą demona, dodanie identyfikatora PID i informacja, że logi będą zapisywane w imieniu użytkownika
	openlog("Demon", LOG_PID, LOG_USER);

	int fd_src, fd_dst;
	void *src_data;
	void *dst_data;

	// Otwarcie pliku źródłowego tylko do odczytu
	if ((fd_src = open(source, O_RDONLY)) == -1)
	{
		currenTime();
		printf("Nie udało się otworzyć pliku źródłowego");
		syslog(LOG_ERR, "Nie udało się otworzyć pliku źródłowego");
		return EXIT_FAILURE;
	}

	// Otwarcie pliku docelowego z możliwością odczytu i zapisu, oraz utworzenie go jeśli nie istnieje
	if ((fd_dst = open(destination, O_RDWR | O_CREAT, 0644)) == -1)
	{
		currenTime();
		printf("Nie udało się otworzyć pliku docelowego");
		syslog(LOG_ERR, "Nie udało się otworzyć pliku docelowego");
		close(fd_src);
		return EXIT_FAILURE;
	}

	// Ustawienie rozmiaru pliku docelowego na podany w argumencie
	if (ftruncate(fd_dst, fileSize) == -1)
	{
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}
	// Otwarcie pliku zrodlowego do odczytu
	src_data = mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd_src, 0);
	// Sprawdzenie czy udalo sie zaalokowac pamiec i obslugiwanie bledu w przypadku niepowodzenia
	if (src_data == MAP_FAILED)
	{
		// Wypisanie bledu do konsoli i logu systemowego
		currenTime();
		printf("Nie udalo sie zmapowac pliku zrodlowego");
		syslog(LOG_ERR, "Nie udalo sie zmapowac pliku zrodlowego");
		// Zamkniecie otwartych plikow i zwrocenie wartosci niepowodzenia
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}

	// Otwarcie pliku docelowego do zapisu
	dst_data = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, fd_dst, 0);
	// Sprawdzenie czy udalo sie zaalokowac pamiec i obslugiwanie bledu w przypadku niepowodzenia
	if (dst_data == MAP_FAILED)
	{
		// Wypisanie bledu do konsoli i logu systemowego
		currenTime();
		printf("Nie udalo sie zmapowac pliku docelowego");
		syslog(LOG_ERR, "Nie udalo sie zmapowac pliku docelowego");
		// Zwolnienie zaalokowanej pamieci, zamkniecie otwartych plikow i zwrocenie wartosci niepowodzenia
		munmap(src_data, fileSize);
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}

	// Skopiowanie danych z pliku zrodlowego do pliku docelowego
	memcpy(dst_data, src_data, fileSize);

	// Zwolnienie zaalokowanej pamieci
	munmap(src_data, fileSize);
	munmap(dst_data, fileSize);

	// Zamkniecie otwartych plikow i logu systemowego
	close(fd_src);
	close(fd_dst);
	closelog();

	// Zwrocenie wartosci powodzenia
	return EXIT_SUCCESS;
}

void compareDestSrc(char *sourcePath, char *destinationPath)
{
	// Inicjalizacja logu systemowego
	openlog("Demon", LOG_PID, LOG_USER);

	// Otwarcie katalogu zrodlowego i katalogu docelowego
	DIR *sourceDir = opendir(sourcePath);
	DIR *destinationDir = opendir(destinationPath);

	// Sprawdzenie czy udalo sie otworzyc katalog zrodlowy i obslugiwanie bledu w przypadku niepowodzenia
	if (sourceDir == NULL)
	{
		// Wypisanie bledu do konsoli i logu systemowego
		currenTime();
		printf("Wystapil problem przy otwieraniu katalogu zrodlowego\n");
		syslog(LOG_ERR, "Wystapil problem przy otwieraniu katalogu zrodlowego\n");
		return;
	}

	// Sprawdzenie czy udalo sie otworzyc katalog docelowy i obslugiwanie bledu w przypadku niepowodzenia
	if (destinationDir == NULL)
	{
		// Wypisanie bledu do konsoli i logu systemowego
		currenTime();
		printf("Wystapil problem przy otwieraniu katalogu docelowego\n");
		syslog(LOG_ERR, "Wystapil problem przy otwieraniu katalogu docelowego\n");
		// Zamkniecie katalogu zrodlowego i zwrocenie funkcji
		closedir(sourceDir);
		return;
	}

	// Inicjalizacja struktury przechowujacej wpis w katalogu docelowym
	struct dirent *destinationEntry;

	// Inicjalizacja tablicy przechowujacej pelna sciezke wpisu w katalogu
	char entryPath[PATH_MAX];

	// Pobieranie kolejnych wpisow z katalogu docelowego i przetwarzanie ich w petli
	while ((destinationEntry = readdir(destinationDir)) != NULL)
	{
		// Sprawdzenie czy aktualny wpis jest plikiem regularnym
		if ((destinationEntry->d_type) == DT_REG)
		{
			// Utworzenie pelnej sciezki do pliku z katalogu zrodlowego
			if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, destinationEntry->d_name) >= PATH_MAX)
			{
				// Wypisanie bledu do konsoli i logu systemowego i przerwanie dzialania programu
				currenTime();
				printf("Wystapil problem przy pobieraniu pelnej sciezki pliku zrodlowego. Plik nie zostal odnaleziony\n");
				syslog(LOG_ERR, "Wystapil problem przy pobieraniu pelnej sciezki pliku zrodlowego. Plik nie zostal odnaleziony\n");
				exit(EXIT_FAILURE);
			}
			// Sprawdzenie czy plik zrodlowy istnieje
			if (access(entryPath, F_OK) != 0)
			{
				// Wyczyszczenie tablicy zawierajacej sciezke do pliku
				clearTheArray(entryPath);
				// Utworzenie pelnej sciezki do pliku z katalogu docelowego
				if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
				{
					// Wypisanie bledu do konsoli i logu systemowego i przerwanie dzialania programu
					currenTime();
					printf("Wystapil problem przy pobieraniu pelnej sciezki pliku docelowego. Plik nie zostal odnaleziony\n");
					syslog(LOG_ERR, "Wystapil problem przy pobieraniu pelnej sciezki pliku docelowego. Plik nie zostal odnaleziony\n");
					exit(EXIT_FAILURE);
				}
				// Usuniecie pliku z katalogu docelowego
				if (unlink(entryPath) == 0)
				{
					// Wypisanie informacji o usunietym pliku do logu systemowego i konsoli
					currenTime();
					syslog(LOG_INFO, "Plik: %s zostal pomyslnie usuniety.\n", destinationEntry->d_name);
					printf("Plik: %s zostal pomyslnie usuniety.\n", destinationEntry->d_name);
				}
			}
		}
		// Wyczyszczenie tablicy zawierajacej sciezke do pliku przed przetworzeniem kolejnego wpisu z katalogu
		clearTheArray(entryPath);
	}

	// Zamkniecie katalogow zrodlowego i docelowego oraz logu systemowego
	closedir(sourceDir);
	closedir(destinationDir);
	closelog();
}

void compareSrcDest(char *sourcePath, char *destinationPath)
{
	// Inicjalizacja logowania
	openlog("Demon", LOG_PID, LOG_USER);

	// Otwarcie katalogów źródłowego i docelowego
	DIR *sourceDir = opendir(sourcePath);
	DIR *destinationDir = opendir(destinationPath);

	// Sprawdzenie czy otwarcie katalogu źródłowego powiodło się
	if (sourceDir == NULL)
	{
		currenTime();
		printf("Wystąpił problem podczas próby otwarcia katalogu źródłowego\n");
		syslog(LOG_ERR, "Wystąpił problem podczas próby otwarcia katalogu źródłowego\n");
		return;
	}

	// Sprawdzenie czy otwarcie katalogu docelowego powiodło się
	if (destinationDir == NULL)
	{
		currenTime();
		printf("Wystąpił problem podczas próby otwarcia katalogu docelowego\n");
		syslog(LOG_ERR, "Wystąpił problem podczas próby otwarcia katalogu docelowego\n");
		closedir(sourceDir);
		return;
	}

	// Struktury zawierające informacje o plikach w katalogach źródłowym i docelowym
	struct stat srcFileInfo;
	struct stat destFileInfo;

	// Kontenery na ścieżki do plików w katalogach źródłowym i docelowym
	char srcFilePathContainer[PATH_MAX];
	char destFilePathContainer[PATH_MAX];

	// Wskaźniki na wpisy katalogowe w katalogach źródłowym i docelowym
	struct dirent *sourceEntry;
	struct dirent *destinationEntry;

	// Kontener na pełną ścieżkę do pliku
	char entryPath[PATH_MAX];

	// Kontenery na daty modyfikacji plików
	char modTimeSrc[20];
	char modTimeDest[20];

	// Struktura z czasami modyfikacji pliku źródłowego
	struct utimbuf srcTimes;

	// Otwieramy katalog źródłowy i katalog docelowy
	while ((sourceEntry = readdir(sourceDir)) != NULL)
	{
		// Sprawdzamy, czy plik jest zwykłym plikiem (nie katalogiem)
		if ((sourceEntry->d_type) == DT_REG)
		{
			// Tworzymy ścieżkę do pliku w katalogu docelowym
			if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, sourceEntry->d_name) >= PATH_MAX)
			{
				currenTime();
				printf("Problem wystąpił podczas próby pobrania pełnej ścieżki do pliku docelowego\n");
				syslog(LOG_ERR, "Problem wystąpił podczas próby pobrania pełnej ścieżki do pliku docelowego\n");
				exit(EXIT_FAILURE);
			}
			// Sprawdzamy, czy plik o takiej samej nazwie istnieje już w katalogu docelowym
			if (access(entryPath, F_OK) == 0)
			{
				currenTime();
				syslog(LOG_INFO, "Plik o takiej samej nazwie został znaleziony: %s\n", sourceEntry->d_name);
				printf("Plik o takiej samej nazwie został znaleziony: %s\n", sourceEntry->d_name);
				clearTheArray(entryPath);

				// Tworzymy ścieżkę do pliku w katalogu źródłowym
				if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem wystąpił podczas próby pobrania pełnej ścieżki do pliku źródłowego\n");
					syslog(LOG_ERR, "Problem wystąpił podczas próby pobrania pełnej ścieżki do pliku źródłowego\n");
					exit(EXIT_FAILURE);
				}

				// Pobieramy informacje o pliku z katalogu źródłowego
				if (stat(entryPath, &srcFileInfo) == -1)
				{
					currenTime();
					printf("Problem wystąpił podczas próby pobrania informacji o pliku (z źródła)\n");
					syslog(LOG_ERR, "Problem wystąpił podczas próby pobrania informacji o pliku (z źródła)\n");
					exit(EXIT_FAILURE);
				}

				// Zmieniamy format daty i czasu modyfikacji pliku na czytelny dla użytkownika
				strftime(modTimeSrc, sizeof(modTimeSrc), "%Y-%m-%d %H:%M:%S", localtime(&srcFileInfo.st_mtime));

				// Pętla while iteruje po wszystkich plikach w katalogu docelowym
				// destinationEntry jest wskaźnikiem na strukturę reprezentującą kolejny plik
				// w katalogu docelowym, a readdir() zwraca NULL, kiedy wszystkie pliki zostaną już przeczytane
				while ((destinationEntry = readdir(destinationDir)) != NULL)
				{
					// Sprawdzenie, czy plik jest regularny
					if ((destinationEntry->d_type) == DT_REG)
					{
						// Porównanie nazw plików
						if (strcmp(sourceEntry->d_name, destinationEntry->d_name) == 0)
						{
							// Wyczyszczenie tablicy entryPath
							clearTheArray(entryPath);

							// sprawdzenie, czy długość pełnej ścieżki do pliku źródłowego nie przekracza PATH_MAX
							if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
							{
								// zapisanie czasu wystąpienia problemu i wypisanie komunikatu o błędzie
								currenTime();
								printf("Wystąpił problem podczas próby uzyskania pełnej ścieżki do pliku źródłowego\n");
								syslog(LOG_ERR, "Wystąpił problem podczas próby uzyskania pełnej ścieżki do pliku źródłowego\n");
								// zakończenie programu z kodem błędu
								exit(EXIT_FAILURE);
							}
							// sprawdzenie informacji o pliku docelowym
							if (stat(entryPath, &destFileInfo) == -1)
							{
								// zapisanie czasu wystąpienia problemu i wypisanie komunikatu o błędzie
								currenTime();
								printf("Wystąpił problem podczas próby uzyskania informacji o pliku docelowym\n");
								syslog(LOG_ERR, "Wystąpił problem podczas próby uzyskania informacji o pliku docelowym\n");
								// zakończenie programu z kodem błędu
								exit(EXIT_FAILURE);
							}
							// konwersja czasu modyfikacji pliku docelowego na string
							strftime(modTimeDest, sizeof(modTimeDest), "%Y-%m-%d %H:%M:%S", localtime(&destFileInfo.st_mtime));

							// Sprawdzanie, czy czas modyfikacji pliku w źródle jest różny od czasu modyfikacji pliku w miejscu docelowym
							if (strcmp(modTimeSrc, modTimeDest) != 0)
							{
								// Tworzenie pełnej ścieżki do pliku źródłowego
								if (snprintf(srcFilePathContainer, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
								{
									currenTime();
									printf("Problem occured when trying to get the full path of source file\n");
									syslog(LOG_ERR, "Problem occured when trying to get the full path of source file\n");
									exit(EXIT_FAILURE);
								}
								// Tworzenie pełnej ścieżki do pliku docelowego
								if (snprintf(destFilePathContainer, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
								{
									currenTime();
									printf("Problem occured when trying to get the full path of destination file\n");
									syslog(LOG_ERR, "Problem occured when trying to get the full path of destination file\n");
									exit(EXIT_FAILURE);
								}

								// Wywołanie funkcji kopiującej zawartość pliku źródłowego do pliku docelowego
								copy(srcFilePathContainer, destFilePathContainer, mmapThreshold);

								// Aktualizacja czasu modyfikacji pliku docelowego na czas modyfikacji pliku źródłowego
								currenTime();
								printf("Different modification times: %s\n", sourceEntry->d_name);
								srcTimes.actime = srcFileInfo.st_atime;
								srcTimes.modtime = srcFileInfo.st_mtime;

								// Ustawianie czasów modyfikacji pliku docelowego
								if (utime(destFilePathContainer, &srcTimes) < 0)
								{
									currenTime();
									printf("Problem occured when trying to set times of dest file\n");
									syslog(LOG_ERR, "Problem occured when trying to set times of dest file\n");
									exit(EXIT_FAILURE);
								}
							}
						}
					}
				}
			}
			else
			{
				// Wywołanie funkcji currenTime, która wypisuje aktualną godzinę w logach.
				currenTime();
				// Wypisanie informacji o nieznalezieniu pliku o tej samej nazwie w katalogu źródłowym.
				printf("Nie znaleziono pliku o tej samej nazwie: %s\n", sourceEntry->d_name);
				// Zapisanie informacji do logów systemowych.
				syslog(LOG_INFO, "Nie znaleziono pliku o tej samej nazwie: %s\n", sourceEntry->d_name);

				// Skonstruowanie ścieżki do pliku źródłowego.
				if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					// Wywołanie funkcji currenTime, która wypisuje aktualną godzinę w logach.
					currenTime();
					// Wypisanie informacji o problemie z uzyskaniem pełnej ścieżki do pliku źródłowego.
					printf("Wystąpił problem podczas próby uzyskania pełnej ścieżki do pliku źródłowego\n");
					// Zapisanie informacji do logów systemowych.
					syslog(LOG_ERR, "Wystąpił problem podczas próby uzyskania pełnej ścieżki do pliku źródłowego\n");
					// Zakończenie programu z kodem błędu.
					exit(EXIT_FAILURE);
				}

				// Sprawdzenie informacji o pliku źródłowym.
				if (stat(entryPath, &srcFileInfo) == -1)
				{
					// Wywołanie funkcji currenTime, która wypisuje aktualną godzinę w logach.
					currenTime();
					// Wypisanie informacji o problemie z dostępem do informacji o pliku źródłowym.
					printf("Wystąpił problem podczas próby uzyskania dostępu do informacji o pliku (ze źródła). Plik nie został znaleziony\n");
					// Zapisanie informacji do logów systemowych.
					syslog(LOG_ERR, "Wystąpił problem podczas próby uzyskania dostępu do informacji o pliku (ze źródła). Plik nie został znaleziony\n");
					// Zakończenie programu z kodem błędu.
					exit(EXIT_FAILURE);
				}

				// Sprawdzenie, czy długość ścieżki do pliku źródłowego nie przekracza PATH_MAX
				if (snprintf(srcFilePathContainer, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem wystąpił podczas próby uzyskania pełnej ścieżki do pliku źródłowego. Plik nie został znaleziony\n");
					syslog(LOG_ERR, "Problem wystąpił podczas próby uzyskania pełnej ścieżki do pliku źródłowego. Plik nie został znaleziony\n");
					exit(EXIT_FAILURE);
				}

				// Sprawdzenie, czy długość ścieżki do pliku docelowego nie przekracza PATH_MAX
				if (snprintf(destFilePathContainer, PATH_MAX, "%s/%s", destinationPath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem wystąpił podczas próby uzyskania pełnej ścieżki do pliku docelowego. Plik nie został znaleziony\n");
					syslog(LOG_ERR, "Problem wystąpił podczas próby uzyskania pełnej ścieżki do pliku docelowego. Plik nie został znaleziony\n");
					exit(EXIT_FAILURE);
				}

				// Skopiowanie pliku źródłowego do pliku docelowego z wykorzystaniem mmap lub read/write
				copy(srcFilePathContainer, destFilePathContainer, mmapThreshold);

				// Zapisanie komunikatu o powodzeniu do pliku logów i wyświetlenie go na ekranie
				currenTime();
				syslog(LOG_INFO, "Plik %s został skopiowany pomyślnie.\n", sourceEntry->d_name);
				printf("Plik %s został skopiowany pomyślnie.\n", sourceEntry->d_name);

				// Ustawienie czasu dostępu i modyfikacji pliku docelowego na takie same jak źródłowego
				srcTimes.actime = srcFileInfo.st_atime;
				srcTimes.modtime = srcFileInfo.st_mtime;

				// Ustawienie czasów dostępu i modyfikacji pliku docelowego przy użyciu funkcji utime
				if (utime(destFilePathContainer, &srcTimes) < 0)
				{
					currenTime();
					printf("Problem wystąpił podczas próby ustawienia czasów dostępu i modyfikacji pliku docelowego\n");
					syslog(LOG_ERR, "Problem wystąpił podczas próby ustawienia czasów dostępu i modyfikacji pliku docelowego\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		// Wyczyszczenie tablic po każdej iteracji pętli oraz ponowne otwarcie katalogu docelowego
		clearTheArray(srcFilePathContainer);
		clearTheArray(destFilePathContainer);
		clearTheArray(modTimeSrc);
		clearTheArray(modTimeDest);
		clearTheArray(entryPath);
		rewinddir(destinationDir);
	}

	// Zamknięcie katalogów źródłowego i docelowego, wywołanie funkcji compareDestSrc oraz zamknięcie pliku logów
	closedir(sourceDir);
	closedir(destinationDir);
	compareDestSrc(sourcePath, destinationPath);
	closelog();
}

void recursiveSynchronization(char *srcPath, char *dstPath)
{
	// Wywolujemy funkcje do kopiowania oraz synchronizacji katalogow i plikow
	copyDirectory(srcPath, dstPath);
	syncDirectory(srcPath, dstPath);
}

void Demon(char **argv)
{
	// Odpowiednio od opcje -r wywolujemy funkcje
	if (recursive)
		recursiveSynchronization(argv[1], argv[2]);
	else
		compareSrcDest(argv[1], argv[2]);
}

void options(int argc, char **argv)
{

	// Iterujemy po tablicy argumentow i odpowiednio ustawiamy wartosci zmiennych globalnych
	for (int i = 3; i < argc; i++)
		if (strcmp(argv[i], "-r") == 0)
			recursive = true;
		else if (strcmp(argv[i], "-t") == 0)
			timeSleep = atoi(argv[i + 1]);
		else if (strcmp(argv[i], "-d") == 0)
			mmapThreshold = atoi(argv[i + 1]);
}

void sigusr1_handler(int signum)
{
	// Uruchomienie logowania dla demona z nazwą "Demon", dodanie identyfikatora procesu oraz określenie źródła logów jako użytkownik systemu
	openlog("Demon", LOG_PID, LOG_USER);

	// Zapisanie bieżącego czasu do logów i wypisanie informacji na ekranie o obudzeniu demona przez sygnał SIGUSR1
	currenTime();
	syslog(LOG_INFO, "Demon obudzony przez sygnał SIGUSR1.\n");
	printf("Demon obudzony przez sygnał SIGUSR1.\n");

	// Ustawienie flagi wymuszającej synchronizację
	forcedSynchro = true;

	// Zamknięcie logowania
	closelog();
}

void createDemon()
{
	// Otwieranie logów systemowych z etykietą "Demon", z opcją dołączenia PID oraz opcją identyfikacji użytkownika
	openlog("Demon", LOG_PID, LOG_USER);

	// Tworzenie procesu potomnego
	pid_t pid, sid;
	pid = fork();
	if (pid < 0) // Proces potomny nie został utworzony
	{
		exit(EXIT_FAILURE);
	}
	if (pid > 0) // Proces macierzysty kończy działanie, aby proces potomny mógł działać niezależnie
	{
		exit(EXIT_SUCCESS);
	}

	// Ustawianie maski plików, aby demon nie ograniczał dostępu do plików
	umask(0);

	// Tworzenie nowej sesji dla procesu potomnego
	sid = setsid();
	if (sid < 0) // Błąd podczas tworzenia sesji
	{
		exit(EXIT_FAILURE);
	}

	// Wyświetlanie informacji o PID demonu w konsoli i logach systemowych
	currenTime();
	printf("PID Procesu Demona: %d.\n", getpid());
	syslog(LOG_INFO, "PID Procesu Demona: %d.\n", getpid());

	// Zamykanie standardowych deskryptorów plików
	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	// Zamykanie logów systemowych
	closelog();
}

int main(int argc, char **argv)
{
	openlog("Demon", LOG_PID, LOG_USER);
	syslog(LOG_INFO, " ");
	syslog(LOG_INFO, "---------------------------------\n");
	syslog(LOG_INFO, "------------>>>Hello<<<----------\n");
	syslog(LOG_INFO, "---------->>>DAEMONN!!<<<--------\n");
	syslog(LOG_INFO, "---------------------------------\n");

	// Sprawdzenie, czy użytkownik podał poprawną ilość argumentów wejściowych
	if (argc < 3)
	{
		currenTime();
		syslog(LOG_INFO, "Nieprawidłowa ilość argumentów wejściowych.\nPrawidłowe użycie: ./demon [KatalogŹródłowy] [KatalogDocelowy]\nOPCJE: -r [SynchronizacjaRekursywna] -t [CzasUśpienia] -d [PrógDzielącyDużePliki]");
		printf("Nieprawidłowa ilość argumentów wejściowych.\nPrawidłowe użycie: ./demon [KatalogŹródłowy] [KatalogDocelowy]\nOPCJE: -r [SynchronizacjaRekursywna] -t [CzasUśpienia] -d [PrógDzielącyDużePliki]");
	}
	{
		// Sprawdzenie podanych opcji i przekazanie ich do odpowiedniej zmiennej
		options(argc, argv);

		// Tworzenie demona
		// createDemon();
		currenTime();
		printf("PID Procesu Demona: %d.\n", getpid());

		// Przypisanie funkcji obsługi sygnału SIGUSR1
		signal(SIGUSR1, sigusr1_handler);

		// Pętla nieskończona demona
		while (1)
		{
			// Sprawdzenie, czy demon został obudzony
			if (!forcedSynchro)
			{
				currenTime();
				printf("Demon obudzony.\n");
				syslog(LOG_INFO, "Demon obudzony.\n");
				forcedSynchro = false;
			}

			// Wywołanie funkcji synchronizującej katalogi
			Demon(argv);

			// Wypisanie informacji o usypianiu demona
			currenTime();
			syslog(LOG_INFO, "Demon uśpiony.");
			syslog(LOG_INFO, "---------------------------------\n");
			printf("Demon uśpiony.\n\n");

			// Uśpienie demona na zadany czas
			sleep(timeSleep);
		}
	}
	closelog();
}
