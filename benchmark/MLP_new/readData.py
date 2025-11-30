import numpy as np
from sklearn.datasets import load_breast_cancer,load_iris
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score,r2_score
from sklearn.preprocessing import OneHotEncoder
import matplotlib.pyplot as plt
import json
import pandas as pd
import subprocess

data = pd.read_csv('./wine.csv')
print(data)
target_data = data['Class']
feature_data = data.drop(columns='Class')
scaler = StandardScaler()
feature_data = scaler.fit_transform(feature_data)
x_train, x_test, y_train, y_test = train_test_split(feature_data, target_data, test_size=0.2, random_state=2)
y_train,y_test=np.array(y_train),np.array(y_test)
data_dict = {'x_train': x_train.tolist(), 'x_test': x_test.tolist(), 'y_train': y_train.tolist(), 'y_test': y_test.tolist()}
with open('temp_data.json', 'w') as f:
    json.dump(data_dict, f)