#!/usr/bin/python

import os
import openai
import sys


# Load your API key from an environment variable or secret management service
openai.api_key = "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"

response = openai.Completion.create(model="text-davinci-003", prompt=sys.argv[1], temperature=1, max_tokens=100)

#response2 = openai.Completion.create(
#    model="curie:ft-personal-2023-02-22-01-11-47",
#    prompt="What is Gruesome Killer?",
#    temperature=0.1,
#    max_tokens=100
#)


print(response.choices[0].text.lstrip())
