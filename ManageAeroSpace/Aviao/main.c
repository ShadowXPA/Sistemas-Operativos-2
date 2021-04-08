#include <tchar.h>
#include <stdlib.h>
#include <Windows.h>

#define MAX_NAME 50

typedef struct airport {
	unsigned int id;					// 1 ~ 90
	unsigned int active : 1;
	TCHAR name[MAX_NAME];
	int x, y;							// 0 ~ 1000
} Airport;

typedef struct airplane {
	unsigned int id;					// 91 ~ 190
	unsigned int active : 1;
	TCHAR name[MAX_NAME];
	int velocity;
	int capacity;						// Current capacity
	int max_capacity;					// Maximum capacity
	int x, y;							// 0 ~ 1000
	unsigned int airport_start;
	unsigned int airport_end;
} Airplane;

int _tmain(int argc, TCHAR **argv, TCHAR **envp) {
	if (argc != 3) {
		return -1;
	}

	Airplane airplane;
	airplane.max_capacity = _ttoi(argv[0]);
	airplane.velocity = _ttoi(argv[1]);
	airplane.airport_start = _ttoi(argv[2]);

	int start = 0; //verifica se a viagem esta em andamento = 1 & 0 se nao estiver em andamento
	TCHAR str[MAX_NAME];
	_tprintf(_T("Introduza o comando que deseja:\n > "));
	_tscanf(_T("%s"), str);
	Ler_cmd(str, start, airplane);
	return 0;
}


void Ler_cmd(TCHAR comand[MAX_NAME], int start, Airplane airplane){
	if(_tcsicmp(comand, "definir destino") == 0 && start == 0){
		DefineDestiny(airplane);
	} else if (_tcsicmp(comand, "embarcar passageiros") == 0 && start == 0) {
		BoardPassengers();
	} else if (_tcsicmp(comand, "iniciar viagem") == 0 && start == 0) {
		StartTrip(start);
	} else if (_tcsicmp(comand, "terminar") == 0) {
		Finish();
	}
}

void DefineDestiny(Airplane airplane){
	TCHAR destiny[MAX_NAME];
	_tprintf(_T("Introduza o nome do próximo destino:\n > "));
	_tscanf(_T("%s"), destiny);

	//funcao para ir buscar o id do aeroporto com o nome
	 FIndPlaneByIDairplane.airport_end = ;
}

void BoardPassengers(){

}

void StartTrip(int start){

	start = 1;
}

void Finish(){

}
