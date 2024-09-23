# Copyright Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.

from dataclasses import dataclass

from transformers import AutoTokenizer


class Internlm2Tokenizer:
    def Init(self):
        use_fast = True
        try:
            self.tokenizer = AutoTokenizer.from_pretrained(
                self.tokenizer_path,
                revision=self.revision,
                padding_side="left",
                truncation_side="left",
                trust_remote_code=self.trust_remote_code,
                use_fast=use_fast
            )
            print(f"use fast tokenizer.")
        except:
            use_fast = False
            self.tokenizer = AutoTokenizer.from_pretrained(
                self.tokenizer_path,
                revision=self.revision,
                padding_side="left",
                truncation_side="left",
                trust_remote_code=self.trust_remote_code,
                use_fast=use_fast
            )
            print(f"use slow tokenizer.")
    
    def Process(self, text):
        input_ids = self.tokenizer([text,], return_tensors="pt", truncation=True)["input_ids"]
        return input_ids
        
    def Destroy(self):
        print(f"now destroy...")
        