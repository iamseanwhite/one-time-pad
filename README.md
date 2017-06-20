# one-time-pad
one-time-pad encryption/decryption system using sockets

Run compileall.

Use keygen keylength to generate a key:
$ keygen 100 > keyFile

Start deamons on different ports in the backgorund:
$ otp_enc_d 54321 &
$ otp_dec_d 54320 &
 
To encrypt and decrypt data: 
$ otp_enc plainTextFile keyFile 57171 > cipherTextFile
$ otp_dec cipherTextFile keyFile 57172 > plainTextFile_a
