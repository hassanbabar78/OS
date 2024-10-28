#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to perform RLE encoding
char* rle_encrypt(const char* input_string) {
    int length = strlen(input_string);
    if (length == 0) return ""; // Handle empty input

    // Allocate maximum size for the output string
    // Worst-case scenario: every character is different (2 * length + 1 for null terminator)
    char* output_string = malloc(2 * length + 1);
    if (!output_string) return NULL; // Memory allocation failure

    int j = 0; // Index for the output string
    int i = 0;

    while (i < length) {
        // Count occurrences of the current character
        int count = 1;
        while (i < length - 1 && input_string[i] == input_string[i + 1]) {
            count++;
            i++;
        }
        // Append the character and its count to the output string
        j += sprintf(&output_string[j], "%c%d", input_string[i], count);
        i++;
    }
    
    output_string[j] = '\0'; // Null-terminate the output string
    return realloc(output_string, j + 1); // Resize to the actual size and return
}
char* rle_decode(const char* encoded_data) {
    // This function will decode the RLE data back to the original data
    // Allocate memory for decoded data (you might need to adjust size logic)
    char* decoded_data = malloc(1024 * 1024); // Example: adjust as needed
    if (!decoded_data) return NULL;

    int i = 0, j = 0;

    while (encoded_data[i] != '\0') {
        char ch = encoded_data[i++];
        int count = 0;

        // Read the number of times the character is repeated
        while (isdigit(encoded_data[i])) {
            count = count * 10 + (encoded_data[i] - '0');
            i++;
        }

        // Repeat the character `count` times in the decoded data
        for (int k = 0; k < count; k++) {
            decoded_data[j++] = ch;
        }
    }

    decoded_data[j] = '\0'; // Null-terminate the decoded data
    return realloc(decoded_data, j + 1); // Resize to the actual size
}

int main() {
    const char* input = "hello this is hamza"; // Example input
    char* encoded = rle_encrypt(input);
    const char* output = encoded;
    char* decoded = rle_decode(output);
    
    // if (encoded) {
    //     printf("Original: %s\n", input);
    //     printf("Encoded: %s\n", encoded);
    //     free(encoded); // Free allocated memory
    // } 
    if (decoded) {
        printf("Original: %s\n", output);
        printf("Decoded: %s\n", decoded);
        free(encoded); // Free allocated memory
    }else {
        printf("Memory allocation failed.\n");
    }

    return 0;
}
