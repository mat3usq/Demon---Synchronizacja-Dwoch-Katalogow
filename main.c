#include "functions.h"

void changeModTime(char *srcFilePath)
{
	char *file_path = srcFilePath;

	time_t now = time(NULL);

	struct utimbuf new_times;
	new_times.actime = now;
	new_times.modtime = now;

	utime(file_path, &new_times);
}

void currenTime()
{
	openlog("Demon", LOG_PID, LOG_USER);

	time_t current_time = time(NULL);
	struct tm *local_time = localtime(&current_time);
	char datetime[21];
	strftime(datetime, 21, "%Y-%m-%d %H:%M:%S", local_time);
	//syslog(LOG_INFO, "[%s] ", datetime);
	printf("[%s] ", datetime);

	closelog();
}

void clearTheArray(char *arr)
{
	size_t length = strlen(arr);
	memset(arr, '\0', length);
}

void copyDirectory(const char *srcPath, const char *dstPath)
{
	openlog("Demon", LOG_PID, LOG_USER);

	DIR *srcDirectory = opendir(srcPath);
	if (srcDirectory == NULL)
	{
		currenTime();
		printf("Error opening source directory.");
		syslog(LOG_ERR, "Error opening source directory.");
		exit(EXIT_FAILURE);
	}
	if (mkdir(dstPath, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1 && errno != EEXIST)
	{
		currenTime();
		printf("Error creating destination directory.");
		syslog(LOG_ERR, "Error creating destination directory.");
		exit(EXIT_FAILURE);
	}
	struct dirent *srcFileInfo;
	while ((srcFileInfo = readdir(srcDirectory)) != NULL)
	{
		if (strcmp(srcFileInfo->d_name, ".") == 0 || strcmp(srcFileInfo->d_name, "..") == 0)
		{
			continue;
		}

		char srcFilePath[PATH_MAX];
		char dstFilePath[PATH_MAX];
		char *src = srcFileInfo->d_name;

		snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, srcFileInfo->d_name);
		snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, srcFileInfo->d_name);

		struct stat srcFileInfo;
		struct stat dstFileInfo;
		char modTimeSrc[20];
		char modTimeDst[20];

		if (lstat(srcFilePath, &srcFileInfo) == -1)
		{
			currenTime();
			printf("Error reading source file/directory statistics.");
			syslog(LOG_ERR, "Error reading source file/directory statistics.");
			syslog(LOG_ERR, "%s\n", srcFilePath);
			syslog(LOG_ERR, "%s\n", dstFilePath);
			exit(EXIT_FAILURE);
		}
		strftime(modTimeSrc, sizeof(modTimeSrc), "%Y-%m-%d %H:%M:%S", localtime(&srcFileInfo.st_mtime));

		if (lstat(dstFilePath, &dstFileInfo) == -1)
		{
			if (S_ISREG(srcFileInfo.st_mode))
			{ // plik
				if (strcmp(modTimeSrc, modTimeDst) != 0)
				{
					copy(srcFilePath, dstFilePath, mmapThreshold);
					changeModTime(srcFilePath);
					currenTime();
					syslog(LOG_INFO, "Different modification times: %s\n", src);
					printf("Different modification times: %s\n", src);
					currenTime();
					syslog(LOG_INFO, "File: %s was successfully copied.\n", src);
					printf("File: %s was successfully copied.\n", src);
				}
				else
				{
					currenTime();
					syslog(LOG_INFO, "File with the same name was found: %s\n", src);
					printf("File with the same name was found: %s\n", src);
				}
			}
			else if (S_ISDIR(srcFileInfo.st_mode))
			{ // katalog
				copyDirectory(srcFilePath, dstFilePath);
				currenTime();
				syslog(LOG_INFO, "Directory: %s was detected.\n", src);
				printf("Directory: %s was detected.\n", src);
			}
		}
		else
		{
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
						syslog(LOG_INFO, "Different modification times: %s\n", src);
						printf("Different modification times: %s\n", src);
						currenTime();
						syslog(LOG_INFO, "File: %s was successfully copied.\n", src);
						printf("File: %s was successfully copied.\n", src);
					}
					else
					{
						currenTime();
						printf("File with the same name was found: %s\n", src);
						syslog(LOG_INFO, "File with the same name was found: %s\n", src);
					}
				}
				else if (S_ISDIR(srcFileInfo.st_mode))
				{ // katalog
					copyDirectory(srcFilePath, dstFilePath);
					currenTime();
					printf("Directory: %s was detected.\n", src);
					syslog(LOG_INFO, "Directory: %s was detected.\n", src);
				}
			}
		}
	}
	if (closedir(srcDirectory) == -1)
	{
		currenTime();
		printf("Error closing source directory.");
		syslog(LOG_ERR, "Error closing source directory.");
		exit(EXIT_FAILURE);
	}

	closelog();
}

void syncDirectory(const char *srcPath, const char *dstPath)
{
	openlog("Demon", LOG_PID, LOG_USER);

	DIR *dstDirectory = opendir(dstPath);
	if (dstDirectory == NULL)
	{
		currenTime();
		printf("Error opening destination directory.");
		syslog(LOG_ERR, "Error opening destination directory.");
		exit(EXIT_FAILURE);
	}
	struct dirent *FileOrDirectory;
	while ((FileOrDirectory = readdir(dstDirectory)) != NULL)
	{
		if (strcmp(FileOrDirectory->d_name, ".") == 0 || strcmp(FileOrDirectory->d_name, "..") == 0)
		{
			continue;
		}
		char srcFilePath[PATH_MAX];
		char dstFilePath[PATH_MAX];
		snprintf(srcFilePath, sizeof(srcFilePath), "%s/%s", srcPath, FileOrDirectory->d_name);
		snprintf(dstFilePath, sizeof(dstFilePath), "%s/%s", dstPath, FileOrDirectory->d_name);
		struct stat FileOrDirectoryInfo;
		if (lstat(dstFilePath, &FileOrDirectoryInfo) == -1)
		{
			currenTime();
			printf("Error reading target file/directory statistics.");
			syslog(LOG_ERR, "Error reading target file/directory statistics.");
			exit(EXIT_FAILURE);
		}
		if (S_ISREG(FileOrDirectoryInfo.st_mode))
		{
			// Jeśli to plik regularny, sprawdź czy istnieje w katalogu źródłowym
			if (access(srcFilePath, F_OK) == -1)
			{
				currenTime();
				printf("File: %s was successfully deleted.\n", FileOrDirectory->d_name);
				syslog(LOG_INFO, "File: %s was successfully deleted.\n", FileOrDirectory->d_name);
				if (unlink(dstFilePath) == -1)
				{
					currenTime();
					printf("Deleting target file error.");
					syslog(LOG_ERR, "Deleting target file error.");
					exit(EXIT_FAILURE);
				}
			}
		}
		else if (S_ISDIR(FileOrDirectoryInfo.st_mode))
		{
			if (access(srcFilePath, F_OK) == -1)
			{
				currenTime();
				printf("Directory: %s was successfully deleted.\n", FileOrDirectory->d_name);
				syslog(LOG_INFO, "Directory: %s was successfully deleted.\n", FileOrDirectory->d_name);
				if (rmdir(dstFilePath) == -1)
				{
					if (errno == ENOTEMPTY)
					{
						syncDirectory("", dstFilePath);
						if (rmdir(dstFilePath) == -1)
						{
							currenTime();
							printf("Deleting target directory error.");
							syslog(LOG_ERR, "Deleting target directory error.");
							exit(EXIT_FAILURE);
						}
					}
					else
					{
						currenTime();
						printf("Deleting target directory error.");
						syslog(LOG_ERR, "Deleting target directory error.");
						exit(EXIT_FAILURE);
					}
				}
			}
			else
			{
				syncDirectory(srcFilePath, dstFilePath);
			}
		}
	}
	if (closedir(dstDirectory) == -1)
	{
		currenTime();
		printf("Error closing destination directory.");
		syslog(LOG_ERR, "Error closing destination directory.");
		exit(EXIT_FAILURE);
	}

	closelog();
}

int copy(char *source, char *destination, int mmapThreshold)
{
	struct stat fileStat;

	if (stat(source, &fileStat) == -1)
	{
		printf("Failed on stat");
	}

	long int fileSize = fileStat.st_size;
	int status;

	if (fileSize > mmapThreshold)
	{
		status = copyUsingMMapWrite(source, destination, fileSize);
	}
	else
	{
		copyUsingReadWrite(source, destination, fileSize);
	}

	if (status != EXIT_SUCCESS)
	{
		return status;
	}

	return EXIT_SUCCESS;
}

void copyUsingReadWrite(const char *srcPath, const char *dstPath, long int bufferSize)
{
	openlog("Demon", LOG_PID, LOG_USER);

	int srcFile = open(srcPath, O_RDONLY);
	if (srcFile == -1)
	{
		currenTime();
		printf("Error opening source file.");
		syslog(LOG_ERR, "Error opening source file.");
		exit(EXIT_FAILURE);
	}
	int dstFile = open(dstPath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (dstFile == -1)
	{
		currenTime();
		printf("Target file open error.");
		syslog(LOG_ERR, "Target file open error.");
		exit(EXIT_FAILURE);
	}
	char buf[bufferSize];
	ssize_t bytesRead, bytesWritten;
	while ((bytesRead = read(srcFile, buf, sizeof(buf))) > 0)
	{
		bytesWritten = write(dstFile, buf, bytesRead);
		if (bytesWritten == -1)
		{
			currenTime();
			printf("Error writing to target file.");
			syslog(LOG_ERR, "Error writing to target file.");
			exit(EXIT_FAILURE);
		}
	}
	if (bytesRead == -1)
	{
		currenTime();
		printf("Source file read error.");
		syslog(LOG_ERR, "Source file read error.");
		exit(EXIT_FAILURE);
	}
	if (close(srcFile) == -1 || close(dstFile) == -1)
	{
		currenTime();
		printf("File close error.");
		syslog(LOG_ERR, "File close error.");
		exit(EXIT_FAILURE);
	}

	closelog();
}

int copyUsingMMapWrite(char *source, char *destination, long int fileSize)
{
	openlog("Demon", LOG_PID, LOG_USER);

	int fd_src, fd_dst;
	void *src_data;
	void *dst_data;

	if ((fd_src = open(source, O_RDONLY)) == -1)
	{
		currenTime();
		printf("Failed to open source file");
		syslog(LOG_ERR, "Failed to open source file");
		return EXIT_FAILURE;
	}

	if ((fd_dst = open(destination, O_RDWR | O_CREAT, 0644)) == -1)
	{
		currenTime();
		printf("Failed to open destination file");
		syslog(LOG_ERR, "Failed to open destination file");
		close(fd_src);
		return EXIT_FAILURE;
	}

	if (ftruncate(fd_dst, fileSize) == -1)
	{
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}

	src_data = mmap(NULL, fileSize, PROT_READ, MAP_SHARED, fd_src, 0);
	if (src_data == MAP_FAILED)
	{
		currenTime();
		printf("Failed to map source file");
		syslog(LOG_ERR, "Failed to map source file");
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}

	dst_data = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, fd_dst, 0);
	if (dst_data == MAP_FAILED)
	{
		currenTime();
		printf("Failed to map destination file");
		syslog(LOG_ERR, "Failed to map destination file");
		munmap(src_data, fileSize);
		close(fd_src);
		close(fd_dst);
		return EXIT_FAILURE;
	}

	memcpy(dst_data, src_data, fileSize);

	munmap(src_data, fileSize);
	munmap(dst_data, fileSize);
	close(fd_src);
	close(fd_dst);
	closelog();

	return EXIT_SUCCESS;
}

void compareDestSrc(char *sourcePath, char *destinationPath)
{
	openlog("Demon", LOG_PID, LOG_USER);
	DIR *sourceDir = opendir(sourcePath);
	DIR *destinationDir = opendir(destinationPath);

	if (sourceDir == NULL)
	{
		currenTime();
		printf("Problem occured when trying to open the source directory\n");
		syslog(LOG_ERR, "Problem occured when trying to open the source directory\n");
		return;
	}
	if (destinationDir == NULL)
	{
		currenTime();
		printf("Problem occured when trying to open the destination directory\n");
		syslog(LOG_ERR, "Problem occured when trying to open the destination directory\n");
		closedir(sourceDir);
		return;
	}

	struct dirent *destinationEntry;

	char entryPath[PATH_MAX];
	while ((destinationEntry = readdir(destinationDir)) != NULL)
	{
		if ((destinationEntry->d_type) == DT_REG)
		{
			if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, destinationEntry->d_name) >= PATH_MAX)
			{
				currenTime();
				printf("Problem occured when trying to get the full path of source file. File was not found\n");
				syslog(LOG_ERR, "Problem occured when trying to get the full path of source file. File was not found\n");
				exit(EXIT_FAILURE);
			}
			if (access(entryPath, F_OK) != 0)
			{
				clearTheArray(entryPath);
				if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem occured when trying to get the full path of destination file. File was not found\n");
					syslog(LOG_ERR, "Problem occured when trying to get the full path of destination file. File was not found\n");
					exit(EXIT_FAILURE);
				}
				if (unlink(entryPath) == 0)
				{
					currenTime();
					syslog(LOG_INFO, "File: %s was successfully deleted.\n", destinationEntry->d_name);
					printf("File: %s was successfully deleted.\n", destinationEntry->d_name);
				}
			}
		}
		clearTheArray(entryPath);
	}
	closedir(sourceDir);
	closedir(destinationDir);
	closelog();
}

void compareSrcDest(char *sourcePath, char *destinationPath)
{
	openlog("Demon", LOG_PID, LOG_USER);

	DIR *sourceDir = opendir(sourcePath);
	DIR *destinationDir = opendir(destinationPath);

	if (sourceDir == NULL)
	{
		currenTime();
		printf("Problem occured when trying to open the source directory\n");
		syslog(LOG_ERR, "Problem occured when trying to open the source directory\n");
		return;
	}
	if (destinationDir == NULL)
	{
		currenTime();
		printf("Problem occured when trying to open the destination directory\n");
		syslog(LOG_ERR, "Problem occured when trying to open the destination directory\n");
		closedir(sourceDir);
		return;
	}

	struct stat srcFileInfo;
	struct stat destFileInfo;

	char srcFilePathContainer[PATH_MAX];
	char destFilePathContainer[PATH_MAX];

	struct dirent *sourceEntry;
	struct dirent *destinationEntry;

	char entryPath[PATH_MAX];

	char modTimeSrc[20];
	char modTimeDest[20];

	struct utimbuf srcTimes;
	while ((sourceEntry = readdir(sourceDir)) != NULL)
	{
		if ((sourceEntry->d_type) == DT_REG)
		{
			if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, sourceEntry->d_name) >= PATH_MAX)
			{
				currenTime();
				printf("Problem occured when trying to get the full path of destination file\n");
				syslog(LOG_ERR, "Problem occured when trying to get the full path of destination file\n");
				exit(EXIT_FAILURE);
			}
			if (access(entryPath, F_OK) == 0)
			{
				currenTime();
				syslog(LOG_INFO, "File with the same name was found: %s\n", sourceEntry->d_name);
				printf("File with the same name was found: %s\n", sourceEntry->d_name);
				clearTheArray(entryPath);
				if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem occured when trying to get the full path of source file\n");
					syslog(LOG_ERR, "Problem occured when trying to get the full path of source file\n");
					exit(EXIT_FAILURE);
				}
				if (stat(entryPath, &srcFileInfo) == -1)
				{
					currenTime();
					printf("Problem occured when trying to access the file info(from source)\n");
					syslog(LOG_ERR, "Problem occured when trying to access the file info(from source)\n");
					exit(EXIT_FAILURE);
				}
				strftime(modTimeSrc, sizeof(modTimeSrc), "%Y-%m-%d %H:%M:%S", localtime(&srcFileInfo.st_mtime));
				while ((destinationEntry = readdir(destinationDir)) != NULL)
				{
					if ((destinationEntry->d_type) == DT_REG)
					{
						if (strcmp(sourceEntry->d_name, destinationEntry->d_name) == 0)
						{
							clearTheArray(entryPath);
							if (snprintf(entryPath, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
							{
								currenTime();
								printf("Problem occured when trying to get the full path of source file\n");
								syslog(LOG_ERR, "Problem occured when trying to get the full path of source file\n");
								exit(EXIT_FAILURE);
							}
							if (stat(entryPath, &destFileInfo) == -1)
							{
								currenTime();
								printf("Problem occured when trying to access the file info(from destination)\n");
								syslog(LOG_ERR, "Problem occured when trying to access the file info(from destination)\n");
								exit(EXIT_FAILURE);
							}
							strftime(modTimeDest, sizeof(modTimeDest), "%Y-%m-%d %H:%M:%S", localtime(&destFileInfo.st_mtime));

							if (strcmp(modTimeSrc, modTimeDest) != 0)
							{
								if (snprintf(srcFilePathContainer, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
								{
									currenTime();
									printf("Problem occured when trying to get the full path of source file\n");
									syslog(LOG_ERR, "Problem occured when trying to get the full path of source file\n");
									exit(EXIT_FAILURE);
								}
								if (snprintf(destFilePathContainer, PATH_MAX, "%s/%s", destinationPath, destinationEntry->d_name) >= PATH_MAX)
								{
									currenTime();
									printf("Problem occured when trying to get the full path of destination file\n");
									syslog(LOG_ERR, "Problem occured when trying to get the full path of destination file\n");
									exit(EXIT_FAILURE);
								}
								copy(srcFilePathContainer, destFilePathContainer, mmapThreshold);
								currenTime();
								printf("Different modification times: %s\n", sourceEntry->d_name);
								srcTimes.actime = srcFileInfo.st_atime;
								srcTimes.modtime = srcFileInfo.st_mtime;
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
				currenTime();
				printf("File with the same name wasn't found: %s\n", sourceEntry->d_name);
				syslog(LOG_INFO, "File with the same name wasn't found: %s\n", sourceEntry->d_name);

				if (snprintf(entryPath, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem occured when trying to get the full path of source file\n");
					syslog(LOG_ERR, "Problem occured when trying to get the full path of source file\n");
					exit(EXIT_FAILURE);
				}
				if (stat(entryPath, &srcFileInfo) == -1)
				{
					currenTime();
					printf("Problem occured when trying to access the file info(from source). File was not found\n");
					syslog(LOG_ERR, "Problem occured when trying to access the file info(from source). File was not found\n");
					exit(EXIT_FAILURE);
				}

				if (snprintf(srcFilePathContainer, PATH_MAX, "%s/%s", sourcePath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem occured when trying to get the full path of source file. File was not found\n");
					syslog(LOG_ERR, "Problem occured when trying to get the full path of source file. File was not found\n");
					exit(EXIT_FAILURE);
				}
				if (snprintf(destFilePathContainer, PATH_MAX, "%s/%s", destinationPath, sourceEntry->d_name) >= PATH_MAX)
				{
					currenTime();
					printf("Problem occured when trying to get the full path of destination file. File was not found\n");
					syslog(LOG_ERR, "Problem occured when trying to get the full path of destination file. File was not found\n");
					exit(EXIT_FAILURE);
				}
				copy(srcFilePathContainer, destFilePathContainer, mmapThreshold);
				currenTime();
				syslog(LOG_INFO, "File: %s was successfully copied.\n", sourceEntry->d_name);
				printf("File: %s was successfully copied.\n", sourceEntry->d_name);
				srcTimes.actime = srcFileInfo.st_atime;
				srcTimes.modtime = srcFileInfo.st_mtime;
				if (utime(destFilePathContainer, &srcTimes) < 0)
				{
					currenTime();
					printf("Problem occured when trying to set times of dest file\n");
					syslog(LOG_ERR, "Problem occured when trying to set times of dest file\n");
					exit(EXIT_FAILURE);
				}
			}
		}
		clearTheArray(srcFilePathContainer);
		clearTheArray(destFilePathContainer);
		clearTheArray(modTimeSrc);
		clearTheArray(modTimeDest);
		clearTheArray(entryPath);
		rewinddir(destinationDir);
	}
	closedir(sourceDir);
	closedir(destinationDir);
	compareDestSrc(sourcePath, destinationPath);
	closelog();
}

void recursiveSynchronization(char *srcPath, char *dstPath)
{
	copyDirectory(srcPath, dstPath);
	syncDirectory(srcPath, dstPath);
}

void Demon(char **argv)
{
	if (recursive)
		recursiveSynchronization(argv[1], argv[2]);
	else
		compareSrcDest(argv[1], argv[2]);
}

void options(int argc, char **argv)
{
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
	openlog("Demon", LOG_PID, LOG_USER);
	currenTime();
	syslog(LOG_INFO, "Demon awakening by signal SIGUSR1.\n");
	printf("Demon awakening by signal SIGUSR1.\n");
	forcedSynchro = true;
	closelog();
}

void createDemon()
{
	openlog("Demon", LOG_PID, LOG_USER);

	pid_t pid, sid;
	pid = fork();
	if (pid < 0)
	{
		exit(EXIT_FAILURE);
	}
	if (pid > 0)
	{
		exit(EXIT_SUCCESS);
	}
	umask(0);
	sid = setsid();
	if (sid < 0)
	{
		exit(EXIT_FAILURE);
	}
	currenTime();
	printf("Demon's PID of procession: %d.\n", getpid());
	syslog(LOG_INFO, "Demon's PID of procession: %d.\n", getpid());

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
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

	if (argc < 3)
	{
		currenTime();
		syslog(LOG_INFO, "Incorrect amount of input parameters.\nCorrectly usage: ./demon [SourceDirectory] [DestinationDirectory]\nOPTIONS: -r [RecursivelySynchro] -t [TimeSleep] -d [BigFileDividingThreshold]");
		printf("Incorrect amount of input parameters.\nCorrectly usage: ./demon [SourceDirectory] [DestinationDirectory]\nOPTIONS: -r [RecursivelySynchro] -t [TimeSleep] -d [BigFileDividingThreshold]");
	}	{
		options(argc, argv);
		createDemon();
		signal(SIGUSR1, sigusr1_handler);
		while (1)
		{
			if (!forcedSynchro)
			{
				currenTime();
				printf("Demon awakening.\n");
				syslog(LOG_INFO, "Demon awakening.\n");
				forcedSynchro = false;
			}

			Demon(argv);

			currenTime();
			syslog(LOG_INFO, "Demon sleeps.");
			syslog(LOG_INFO, "---------------------------------\n");
			printf("Demon sleeps.\n\n");
			sleep(timeSleep);
		}
	}
	closelog();
}
