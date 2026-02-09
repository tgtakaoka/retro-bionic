;; platformio cross setting
;; hint: M-x elisp-autofmt-buffer
(put 'lsp-clients-clangd-args 'safe-local-variable #'listp)
((c++-mode
  .
  ((eval
    .
    (setq-local
     lsp-clients-clangd-args
     (list
      "--header-insertion=never" ; stop insert #include automatically
      (concat
       "--query-driver="
       (expand-file-name
        "~/.platformio/packages/toolchain-gccarmnoneeabi-teensy/bin/arm-none-eabi-g++"))))))))
