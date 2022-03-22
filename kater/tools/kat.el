;;; kat.el --- sample major mode for editing kat files. -*- coding: utf-8; lexical-binding: t; -*-

;; Copyright © 2022, Michalis Kokologiannakis

;; Author: Michalis Kokologiannakis (michalis@mpi-sws.org)
;; Version: 0.1
;; Created: 2022.03.22
;; Keywords: languages

;; This file is not part of GNU Emacs.

;;; License:

;; You can redistribute this program and/or modify it under the terms of the GNU General Public License version 2.

;;; Commentary:

;; short description here

;; full doc on how to use here

;;; Code:

;; create the list for font-lock.
;; each category of keyword is given a particular face
(setq kat-font-lock-keywords
      (let* (
            ;; define several category of keywords
            (x-keywords '("assume" "let" "save" "assert" "coherence" "check" "acyclic" "error" "warning"))
            (x-types '("\\[R\\]" "\\[W\\]" "\\[F\\]" "\\[UR\\]" "\\[UW\\]"))
            (x-constants '("po-imm" "mo-imm" "rf" "fr-init" "loc-overlap"))
            (x-events '("unless" "reduce" "attach"))
            (x-functions '("llAbs" "llAcos" "llAddToLandBanList" "llAddToLandPassList"))

            ;; generate regex string for each category of keywords
            (x-keywords-regexp (regexp-opt x-keywords 'words))
            (x-types-regexp (regexp-opt x-types 'words))
            (x-constants-regexp (regexp-opt x-constants 'words))
            (x-events-regexp (regexp-opt x-events 'words))
            (x-functions-regexp (regexp-opt x-functions 'words)))

        `(
          (,x-types-regexp . 'font-lock-type-face)
          (,x-constants-regexp . 'font-lock-constant-face)
          (,x-events-regexp . 'font-lock-builtin-face)
          (,x-functions-regexp . 'font-lock-function-name-face)
          (,x-keywords-regexp . 'font-lock-keyword-face)
          ;; note: order above matters, because once colored, that part won't change.
          ;; in general, put longer words first
          )))

;;;###autoload
(define-derived-mode kat-mode c++-mode "kat mode"
  "Major mode for editing kat files"

  ;; code for syntax highlighting
  (setq font-lock-defaults '((kat-font-lock-keywords))))

;; add the mode to the `features' list
(provide 'kat-mode)

;;; kat-mode.el ends here
