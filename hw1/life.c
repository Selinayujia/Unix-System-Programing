/*life.c
hw1
Conway's well known Game of Life 
author@ Selina Zhang
*/
#include<stdio.h>
#include<string.h>
#include <stdlib.h>
int MAX_PARAMETER = 5; //prevent user gives excessive parameters

int* setCols(char* line, ssize_t line_len, int col_num){ //set the cols that has contents from input
    int *row = (int *)malloc(sizeof(int)*(col_num+2));
    if(row == NULL){
        printf("Memory allocate failed\n");
        exit(1);
    }

    row[0] = 0;
    row[col_num + 1] = 0;   // set the dead cells left of grid and right of grid
    if(line_len < col_num){
        for(int i= 0; i < line_len; ++i){
            
            if(line[i] == '*' ){ //life cell will be represent as 1
                row[i+1] = 1;
            }
            else{
                row[i+1] = 0;
            }
        }
        for(int i = line_len; i< col_num+1; ++i){
            row[i] = 0;
        }
    }
    else{   // line length longer than col, only read the first col_num char from line
        for(int i = 0; i< col_num+1; ++i){
             if(line[i]=='*' ){
                row[i+1] = 1;
            }
            else{
                row[i+1] = 0;
            }
        }
        
    }

    return row;
}

int** setGridRows(int row_num, int col_num, FILE* fp){  // set the rows for the grid
    int **grid = (int **)malloc(sizeof(int*)*(row_num+2)); // two more row for boundary
    if(grid == NULL){
        printf("Memory allocate failed\n");
        exit(1);
    }
    ssize_t line_len;                        // for read line
    char *line = NULL;
    size_t len = 0;
    for(int i = 0; i < row_num+2; ++i){
        int *row = (int *)malloc(sizeof(int)*(col_num+2));  // two more col for boundary
        if(row == NULL){
            printf("Memory allocate failed\n");
            exit(1);
        }
        if (i == 0 || i == row_num+1){     //inicialize dead cells top of the grid, bottom of grid
            for(int j =0; j<col_num+2; ++j){
                row[j] = 0;    // the dead cell is represented as 0
            }
        }
        else{
            int line_count = 1;
            line_len = getline(&line, &len, fp);
            if(line_len != -1){
                row = setCols(line, line_len, col_num);
                line_count += 1;
            }
            else if(line_count < col_num){
                for(int j =0; j<col_num+2; ++j){
                    row[j] = 0;
                }
            }
            else{
                printf("%s", "Can not read line\n");
                exit(1);
                
            }
        }
        grid[i] = row;
    }
    free(line);
    return grid;
}

void setupFromCommand(int* row_num, int* col_num, char** filename, int* gen, int argc, char *argv[]){
    // read in the command line argument
    int loop_ind = argc > MAX_PARAMETER ? MAX_PARAMETER - 1 : argc - 1;  // read the first 5 as valid arguments
    while(loop_ind != 0){
        if(loop_ind == 1){
            *row_num = atoi(argv[1]); // read in row number
        }
        else if(loop_ind == 2){
            *col_num = atoi(argv[2]);  // read in column number
        }
        else if(loop_ind == 3){
            *filename = (char*)malloc(sizeof(char)*(strlen(argv[3])+1));  // read in filename
            if(*filename == NULL){
                printf("Memory allocate failed\n");
                exit(1);
            }
            strcpy(*filename, argv[3]);
            
        }
        else{
            *gen = atoi(argv[4]);   // read in generation number
        }
        loop_ind -= 1;
        
    }
    if(*row_num <= 0 || *col_num <= 0 || *gen <= 0){
        printf("row number, column number and generation must be positive intergers!\n");
        exit(1);
    }
}
void print_grid(int** grid, int row_num, int col_num, int gen){  // print the grid in char
    printf("Generation %d:\n", gen);
    for (int i = 1; i < row_num+1; ++i){
        for (int j = 1; j < col_num+1; ++j){
            grid[i][j] == 1 ? printf("*") :  printf("-"); //if num1 then alive so print *, else -
        }
        printf("\n");
    }
    printf("=========================================\n");
}

int** transform(int** grid, int row_num, int col_num){
    //Any live cell with fewer than two live neighbours dies. (1)
   // Any live cell with more than three live neighbours dies. (2)
   // Any dead cell with exactly three live neighbours becomes a live cell. (3)
    int **newGrid = (int **)malloc(sizeof(int*)*(row_num+2));  //create a new grid, bc modify the old grid one
    if(newGrid == NULL){                            //by one will result the inaccurate for other cell's result
        printf("Memory allocate failed\n");
        exit(1);
    }
    for(int i = 0; i < row_num+2; ++i){
        int* newRow = (int *)malloc(sizeof(int)*(col_num+2));
        if(newRow == NULL){
            printf("Memory allocate failed\n");
            exit(1);
        }
		newRow = grid[i];
        for (int j = 1; j < col_num+1; ++j){
            newRow[j] = grid[i][j];
        }
        newGrid[i] = newRow;
        
    }
    for (int i = 1; i < row_num+1; ++i){
        for (int j = 1; j < col_num+1; ++j){
            int live_neighbour = 0;       //check all neighbours, if live, +1
            if(grid[i][j-1] == 1){live_neighbour+= 1;}
            if(grid[i][j+1] == 1){live_neighbour+= 1;}
            if(grid[i-1][j-1] == 1){live_neighbour+= 1;}
            if(grid[i-1][j] == 1){live_neighbour+= 1;}
            if(grid[i-1][j+1] == 1){live_neighbour+= 1;}
            if(grid[i+1][j-1] == 1) {live_neighbour+= 1;}
            if(grid[i+1][j] == 1){live_neighbour+= 1;}
            if(grid[i+1][j+1] == 1){live_neighbour+= 1;}
            
            if(grid[i][j] == 1 && (live_neighbour < 2 || live_neighbour > 3) ){ //condition 1, 2
                    newGrid[i][j] = 0;
            }
            else if(grid[i][j] == 0 && live_neighbour == 3 ){ //condition 3
                     newGrid[i][j] = 1;
            }

        }
    }
	
        for(int i = 0; i<row_num+2; ++i){
            free(grid[i]);
            grid[i] = NULL;
        }
        free(grid);
        grid = NULL;
   
    return newGrid;
}



int main(int argc, char *argv[]){
	int row_num = 10;
	int col_num = 10;
    char* filename = "life.txt";
	int gen = 10;
    setupFromCommand(&row_num, &col_num, &filename, &gen, argc, argv);
    FILE* fp = fopen(filename,"r");
    if ( fp == 0 ){
        printf( "%s", "Could not open file\n" );
        exit(1);
    }
    else{
        int** grid = setGridRows(row_num,col_num,fp);
        for(int i = 0; i <= gen; ++i){
            print_grid(grid, row_num, col_num, i);
            grid = transform(grid, row_num, col_num);
        }
        // free the memory
        for(int i = 0; i<row_num+2; ++i){
            free(grid[i]);
            grid[i] = NULL;
        }
        free(grid);
        grid = NULL;
       
    }
    return 0;
}
